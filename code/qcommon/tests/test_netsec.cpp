/*
===========================================================================
Copyright (C) 2026 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "../q_shared.h"
#include "../qcommon.h"

#include <cstdio>
#include <cstring>

static int failures;

static void msg_setup(msg_t *msg, byte *data, size_t maxsize, size_t cursize)
{
	std::memset(msg, 0, sizeof(*msg));
	msg->data = data;
	msg->maxsize = maxsize;
	msg->cursize = cursize;
}

static void expect_true(const char *name, qboolean value)
{
	if (!value) {
		std::fprintf(stderr, "FAIL: %s expected true\n", name);
		failures++;
	}
}

static void expect_false(const char *name, qboolean value)
{
	if (value) {
		std::fprintf(stderr, "FAIL: %s expected false\n", name);
		failures++;
	}
}

static void test_path_traversal()
{
	expect_true("null path", COM_IsPathTraversal(NULL));
	expect_true("empty path", COM_IsPathTraversal(""));
	expect_true("dotdot", COM_IsPathTraversal("../secret.pk3"));
	expect_true("nested dotdot", COM_IsPathTraversal("main/../../etc/passwd"));
	expect_true("windows style", COM_IsPathTraversal("main\\..\\secret"));
	expect_true("colon colon", COM_IsPathTraversal("c::/windows"));
	expect_false("plain pak", COM_IsPathTraversal("main/pak0.pk3"));
	expect_false("subdir pak", COM_IsPathTraversal("main/maps/obj/obj_team2.pk3"));
}

static void test_huffman_truncated_connect()
{
	byte data[64];
	msg_t msg;
	size_t before;

	std::memset(data, 0, sizeof(data));
	data[0] = 0xFF;
	data[1] = 0xFF;

	msg_setup(&msg, data, sizeof(data), 8);
	before = msg.cursize;

	Huff_Decompress(&msg, 0);

	if (msg.cursize < before) {
		std::fprintf(stderr, "FAIL: Huff_Decompress shrunk truncated buffer unexpectedly\n");
		failures++;
	}
}

static void test_huffman_roundtrip()
{
	byte src[256];
	byte buf[4096];
	msg_t msg;
	int i;

	for (i = 0; i < (int)sizeof(src); i++) {
		src[i] = (byte)(i & 0x0f);
	}

	std::memset(buf, 0, sizeof(buf));
	std::memcpy(buf, src, sizeof(src));
	msg_setup(&msg, buf, sizeof(buf), sizeof(src));

	Huff_Compress(&msg, 0);
	if (msg.cursize <= sizeof(src) && msg.cursize < 4) {
		std::fprintf(stderr, "FAIL: Huff_Compress produced tiny output %zu\n", msg.cursize);
		failures++;
		return;
	}

	Huff_Decompress(&msg, 0);
	if (msg.cursize < sizeof(src)) {
		std::fprintf(stderr, "FAIL: Huff_Decompress size %zu < %zu\n", msg.cursize, sizeof(src));
		failures++;
		return;
	}

	if (std::memcmp(msg.data, src, sizeof(src)) != 0) {
		std::fprintf(stderr, "FAIL: Huff roundtrip mismatch\n");
		failures++;
	}
}

static void test_huffman_claimed_oversize()
{
	byte data[32];
	msg_t msg;

	std::memset(data, 0xA5, sizeof(data));
	data[0] = 0x10;
	data[1] = 0x00;

	msg_setup(&msg, data, sizeof(data), sizeof(data));

	Huff_Decompress(&msg, 0);

	if (msg.cursize > msg.maxsize) {
		std::fprintf(stderr, "FAIL: Huff_Decompress cursize %zu > maxsize %zu\n", msg.cursize, msg.maxsize);
		failures++;
	}
}

int main()
{
	failures = 0;

	test_path_traversal();
	test_huffman_truncated_connect();
	test_huffman_roundtrip();
	test_huffman_claimed_oversize();

	if (failures) {
		std::fprintf(stderr, "%d netsec test(s) failed\n", failures);
		return 1;
	}

	std::printf("netsec tests passed\n");
	return 0;
}
