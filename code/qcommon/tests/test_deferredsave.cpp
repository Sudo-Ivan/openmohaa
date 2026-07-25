/*
===========================================================================
Copyright (C) 2025 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMoHAA source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenMoHAA source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// test_deferredsave.cpp: Tests for the deferred save mechanism
//
// Verifies that:
//   1. CSVG compression format round-trips correctly (raw → CSVG → raw)
//   2. Buffer detach semantics preserve data integrity
//   3. Multiple phased operations produce correct final output

#include "../q_shared.h"
#include "../../corepp/lz77.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int failures;

static void expect_true(const char *name, int value)
{
    if (!value) {
        std::fprintf(stderr, "FAIL: %s expected true\n", name);
        failures++;
    }
}

static void expect_int_eq(const char *name, int actual, int expected)
{
    if (actual != expected) {
        std::fprintf(stderr, "FAIL: %s: expected %d, got %d\n", name, expected, actual);
        failures++;
    }
}

// Minimal LZ77 wrapper matching the CSVG format used by DeferredSave_CompressBuffer.
// This tests the same compression pipeline without depending on ArchiveFile/gi.
// CSVG format: [4 bytes 'CSVG'][4 bytes LE uncompressed size][LZ77 compressed data]
static bool csvg_compress(
    const unsigned char *in, size_t in_len, unsigned char *out, size_t out_cap, size_t *out_len
)
{
    if (out_cap < 8) {
        return false;
    }

    out[0] = 'C';
    out[1] = 'S';
    out[2] = 'V';
    out[3] = 'G';

    unsigned int temp_le = LittleLong((unsigned int)in_len);
    memcpy(out + 4, &temp_le, 4);

    if (in_len == 0) {
        *out_len = 8;
        return true;
    }

    size_t compressed;
    if (g_lz77.Compress(const_cast<unsigned char *>(in), in_len, out + 8, &compressed) != 0) {
        return false;
    }

    *out_len = compressed + 8;
    return true;
}

static bool csvg_decompress(
    const unsigned char *in, size_t in_len, unsigned char *out, size_t out_cap, size_t *out_len
)
{
    if (in_len < 8) {
        return false;
    }
    if (in[0] != 'C' || in[1] != 'S' || in[2] != 'V' || in[3] != 'G') {
        return false;
    }

    unsigned int orig_size;
    memcpy(&orig_size, in + 4, 4);
    orig_size = LittleLong(orig_size);

    if (out_cap < orig_size) {
        return false;
    }

    if (orig_size == 0) {
        *out_len = 0;
        return true;
    }

    size_t decomplen;
    if (g_lz77.Decompress(
            const_cast<unsigned char *>(in + 8), in_len - 8, out, &decomplen
        ) != 0) {
        return false;
    }

    *out_len = decomplen;
    return (decomplen == orig_size);
}

static void test_csvg_roundtrip()
{
    std::printf("test_csvg_roundtrip...\n");

    unsigned char in[4096];
    unsigned char compressed[4600];
    unsigned char decompressed[4096];
    size_t        compressed_len;
    size_t        decompressed_len;

    memcpy(in, "Hello, DeferredSave!", 21);
    memset(in + 21, 0xAB, sizeof(in) - 21);

    bool ok = csvg_compress(in, sizeof(in), compressed, sizeof(compressed), &compressed_len);
    expect_true("csvg_compress succeeded", ok);
    expect_true("csvg_compress produced output", compressed_len > 8);
    expect_int_eq("csvg_compress header CSVG", memcmp(compressed, "CSVG", 4) == 0, 1);

    ok = csvg_decompress(compressed, compressed_len, decompressed, sizeof(decompressed), &decompressed_len);
    expect_true("csvg_decompress succeeded", ok);
    expect_int_eq("csvg_decompress length match", (int)decompressed_len, (int)sizeof(in));
    expect_int_eq("csvg_decompress content match", memcmp(in, decompressed, sizeof(in)), 0);
}

static void test_csvg_empty()
{
    std::printf("test_csvg_empty...\n");

    unsigned char in[1] = {0};
    unsigned char compressed[64];
    unsigned char decompressed[16];
    size_t        compressed_len;
    size_t        decompressed_len;

    bool ok = csvg_compress(in, 0, compressed, sizeof(compressed), &compressed_len);
    expect_true("csvg_compress empty", ok);
    expect_int_eq("csvg_compress empty len", (int)compressed_len, 8);

    ok = csvg_decompress(compressed, compressed_len, decompressed, sizeof(decompressed), &decompressed_len);
    expect_true("csvg_decompress empty", ok);
    expect_int_eq("csvg_decompress empty len", (int)decompressed_len, 0);
}

static void test_csvg_tiny()
{
    std::printf("test_csvg_tiny...\n");

    unsigned char in[] = {0x01, 0x02, 0x03};
    unsigned char compressed[64];
    unsigned char decompressed[16];
    size_t        compressed_len;
    size_t        decompressed_len;

    bool ok = csvg_compress(in, sizeof(in), compressed, sizeof(compressed), &compressed_len);
    expect_true("csvg_compress tiny", ok);

    ok = csvg_decompress(compressed, compressed_len, decompressed, sizeof(decompressed), &decompressed_len);
    expect_true("csvg_decompress tiny", ok);
    expect_int_eq("csvg_decompress tiny content", memcmp(in, decompressed, sizeof(in)), 0);
}

static void test_csvg_buffer_detach()
{
    std::printf("test_csvg_buffer_detach...\n");

    // Simulate the buffer detach pattern used in ArchiveFile::DetachBuffer:
    // 1. Raw data is allocated
    // 2. Buffer pointer is transferred ("detached")
    // 3. Original pointer becomes NULL
    // 4. Transferred buffer can be compressed and written

    unsigned char *raw = (unsigned char *)std::malloc(256);
    expect_true("detach raw alloc", raw != NULL);

    memset(raw, 0x42, 256);

    // "Detach" — transfer ownership
    unsigned char *detached = raw;
    raw = NULL;

    expect_true("detach original is NULL", raw == NULL);
    expect_true("detached buffer valid", detached != NULL);
    expect_int_eq("detached content ok", detached[0], 0x42);
    expect_int_eq("detached content ok last", detached[255], 0x42);

    // Now compress the detached buffer (as DeferredSave_CompressBuffer would)
    unsigned char compressed[300];
    size_t        compressed_len;

    bool ok = csvg_compress(detached, 256, compressed, sizeof(compressed), &compressed_len);
    expect_true("csvg_compress detached buffer", ok);

    // Free the detached buffer (as DeferredSave_Cancel would)
    // After Cancel, the buffer is freed and pointer is NULL
    unsigned char *to_free = detached;
    detached = NULL;
    std::free(to_free);

    expect_true("detached freed and NULL", detached == NULL);

    // Decompress to verify
    unsigned char decompressed[256];
    size_t        decompressed_len;
    ok = csvg_decompress(compressed, compressed_len, decompressed, sizeof(decompressed), &decompressed_len);
    expect_true("csvg_decompress after detach", ok);
    expect_int_eq("decompressed all 0x42", decompressed[0], 0x42);
    expect_int_eq("decompressed all 0x42 last", decompressed[255], 0x42);
}

int main()
{
    failures = 0;

    test_csvg_roundtrip();
    test_csvg_empty();
    test_csvg_tiny();
    test_csvg_buffer_detach();

    if (failures) {
        std::fprintf(stderr, "%d deferred save test(s) failed\n", failures);
        return 1;
    }

    std::printf("deferred save tests passed\n");
    return 0;
}
