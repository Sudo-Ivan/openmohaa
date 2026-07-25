#include "../q_shared.h"
#include "../qcommon.h"

extern "C" {
#include "../crc.h"
}

#include <cstdio>
#include <cstring>
#include <cmath>

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

static void expect_float_close(const char *name, float actual, float expected, float epsilon)
{
    if (fabs(actual - expected) > epsilon) {
        std::fprintf(stderr, "FAIL: %s: expected %f, got %f\n", name, expected, actual);
        failures++;
    }
}

static void expect_str_eq(const char *name, const char *actual, const char *expected)
{
    if (std::strcmp(actual, expected) != 0) {
        std::fprintf(stderr, "FAIL: %s: expected \"%s\", got \"%s\"\n", name, expected, actual);
        failures++;
    }
}

static void test_crc_oracle()
{
    unsigned short crc;

    CRC_Init(&crc);
    CRC_ProcessByte(&crc, 0x31);
    CRC_ProcessByte(&crc, 0x32);
    CRC_ProcessByte(&crc, 0x33);
    CRC_ProcessByte(&crc, 0x34);
    CRC_ProcessByte(&crc, 0x35);
    CRC_ProcessByte(&crc, 0x36);
    CRC_ProcessByte(&crc, 0x37);
    CRC_ProcessByte(&crc, 0x38);
    CRC_ProcessByte(&crc, 0x39);
    unsigned short check = CRC_Value(crc);

    expect_int_eq("CRC check value for 123456789", (int)check, 0x29B1);

    unsigned char data[] = "123456789";
    unsigned short block_crc = CRC_Block(data, 9);
    expect_int_eq("CRC_Block 123456789", (int)block_crc, 0x29B1);
}

static void test_qrand_oracle()
{
    int seed;

    seed = 0;
    int r1 = Q_rand(&seed);
    expect_int_eq("Q_rand seed=0 first call returns 1", r1, 1);
    expect_int_eq("Q_rand seed becomes 1 after first call", seed, 1);

    int r2 = Q_rand(&seed);
    expect_int_eq("Q_rand seed=1 second call returns 69070", r2, 69070);
    expect_int_eq("Q_rand seed becomes 69070", seed, 69070);

    seed = 0;
    int r1b = Q_rand(&seed);
    expect_int_eq("Q_rand deterministic first", r1b, 1);
    int r2b = Q_rand(&seed);
    expect_int_eq("Q_rand deterministic second", r2b, 69070);
}

static void test_qrandom_oracle()
{
    int seed;

    seed = 0;
    float r = Q_random(&seed);
    expect_float_close("Q_random seed=0", r, 1.0f / 65536.0f, 0.000001f);

    seed = 0;
    r = Q_crandom(&seed);
    expect_float_close("Q_crandom seed=0", r, 2.0f * (1.0f / 65536.0f - 0.5f), 0.000001f);
}

static void test_hashkey_oracle()
{
    char buf[32];

    std::strcpy(buf, "");
    expect_int_eq("HashKey empty string", Com_HashKey(buf, 4), 0);

    std::strcpy(buf, "a");
    expect_int_eq("HashKey 'a'", Com_HashKey(buf, 4), 11543 ^ 11);

    std::strcpy(buf, "ab");
    expect_int_eq("HashKey 'ab'", Com_HashKey(buf, 4), 23303 ^ 22);

    std::strcpy(buf, "test");
    int h = 116 * 119 + 101 * 120 + 115 * 121 + 116 * 122;
    h = h ^ (h >> 10) ^ (h >> 20);
    expect_int_eq("HashKey 'test'", Com_HashKey(buf, 4), h);

    std::strcpy(buf, "test");
    int partial = 116 * 119 + 101 * 120;
    expect_int_eq("HashKey 'test' maxlen=2",
        Com_HashKey(buf, 2), partial ^ (partial >> 10));
}

static void test_endian_oracle()
{
    short s = ShortSwap((short)0x1234);
    expect_int_eq("ShortSwap 0x1234", (int)s, (int)(short)0x3412);

    unsigned short us = UnsignedShortSwap((unsigned short)0xABCD);
    expect_int_eq("UnsignedShortSwap 0xABCD", (int)us, (int)(unsigned short)0xCDAB);

    int l = LongSwap(0x12345678);
    expect_int_eq("LongSwap 0x12345678", l, 0x78563412);

    unsigned int ul = UnsignedLongSwap(0xDEADBEEFu);
    expect_int_eq("UnsignedLongSwap 0xDEADBEEF", (int)ul, (int)0xEFBEADDEu);

    short orig = (short)0xDEAD;
    expect_int_eq("ShortSwap round-trip", (int)ShortSwap(ShortSwap(orig)), (int)orig);

    int lorig = 0xCAFEBABE;
    expect_int_eq("LongSwap round-trip", LongSwap(LongSwap(lorig)), lorig);
}

static void test_vector_oracle()
{
    vec3_t v, out;

    v[0] = 3.0f; v[1] = 4.0f; v[2] = 0.0f;
    float len = VectorNormalize2(v, out);
    expect_float_close("VectorNormalize2 length of (3,4,0)", len, 5.0f, 0.0001f);
    expect_float_close("VectorNormalize2 x of (3,4,0)", out[0], 0.6f, 0.0001f);
    expect_float_close("VectorNormalize2 y of (3,4,0)", out[1], 0.8f, 0.0001f);
    expect_float_close("VectorNormalize2 z of (3,4,0)", out[2], 0.0f, 0.0001f);

    v[0] = 3.0f; v[1] = 4.0f; v[2] = 0.0f;
    expect_float_close("VectorLength (3,4,0)", VectorLength(v), 5.0f, 0.0001f);

    vec3_t a = {1.0f, 0.0f, 0.0f};
    vec3_t b = {0.0f, 1.0f, 0.0f};
    vec3_t c;
    CrossProduct(a, b, c);
    expect_float_close("CrossProduct x (1,0,0)x(0,1,0)", c[0], 0.0f, 0.0001f);
    expect_float_close("CrossProduct y (1,0,0)x(0,1,0)", c[1], 0.0f, 0.0001f);
    expect_float_close("CrossProduct z (1,0,0)x(0,1,0)", c[2], 1.0f, 0.0001f);

    vec3_t p1 = {0.0f, 0.0f, 0.0f};
    vec3_t p2 = {3.0f, 4.0f, 0.0f};
    expect_float_close("Distance (0,0,0)-(3,4,0)", Distance(p1, p2), 5.0f, 0.0001f);
}

static void test_angle_oracle()
{
    expect_float_close("AngleNormalize360 370->10", AngleNormalize360(370.0f), 10.0f, 0.0001f);
    expect_float_close("AngleNormalize360 -10->350", AngleNormalize360(-10.0f), 350.0f, 0.0001f);
    expect_float_close("AngleNormalize180 190->-170", AngleNormalize180(190.0f), -170.0f, 0.0001f);
    expect_float_close("AngleNormalize180 -190->170", AngleNormalize180(-190.0f), 170.0f, 0.0001f);
    expect_float_close("AngleDelta 10,350", AngleDelta(10.0f, 350.0f), -20.0f, 0.0001f);
    expect_float_close("LerpAngle 0,90,0.5", LerpAngle(0.0f, 90.0f, 0.5f), 45.0f, 0.0001f);
    expect_float_close("LerpAngle 350,10,0.5", LerpAngle(350.0f, 10.0f, 0.5f), 0.0f, 0.0001f);
}

static void test_string_oracle()
{
    expect_true("Q_stricmp equal", Q_stricmp("hello", "HELLO") == 0);
    expect_true("Q_stricmp different", Q_stricmp("hello", "world") != 0);
    expect_true("Q_stricmpn prefix match", Q_stricmpn("abcdef", "ABCxyz", 3) == 0);
    expect_true("Q_stricmpn prefix mismatch", Q_stricmpn("abcdef", "abDxyz", 3) != 0);

    char buf[32];
    Q_strncpyz(buf, "hello world", sizeof(buf));
    expect_str_eq("Q_strncpyz exact", buf, "hello world");

    Q_strncpyz(buf, "this is a very long string that should be truncated", sizeof(buf));
    expect_true("Q_strncpyz truncated", std::strlen(buf) == sizeof(buf) - 1);

    char mixed[] = "Hello World";
    Q_strlwr(mixed);
    expect_str_eq("Q_strlwr", mixed, "hello world");
    Q_strupr(mixed);
    expect_str_eq("Q_strupr", mixed, "HELLO WORLD");
}

int main()
{
    failures = 0;

    std::printf("test_crc_oracle...\n");
    test_crc_oracle();

    std::printf("test_qrand_oracle...\n");
    test_qrand_oracle();

    std::printf("test_qrandom_oracle...\n");
    test_qrandom_oracle();

    std::printf("test_hashkey_oracle...\n");
    test_hashkey_oracle();

    std::printf("test_endian_oracle...\n");
    test_endian_oracle();

    std::printf("test_vector_oracle...\n");
    test_vector_oracle();

    std::printf("test_angle_oracle...\n");
    test_angle_oracle();

    std::printf("test_string_oracle...\n");
    test_string_oracle();

    if (failures) {
        std::fprintf(stderr, "%d oracle test(s) failed\n", failures);
        return 1;
    }

    std::printf("oracle tests passed\n");
    return 0;
}
