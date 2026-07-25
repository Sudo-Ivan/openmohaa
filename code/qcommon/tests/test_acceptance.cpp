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

static void test_compress_acceptance()
{
    char src[256];
    int len;

    std::strcpy(src, "key1 value1\n// this is a comment\nkey2 value2\n");
    len = COM_Compress(src);
    expect_true("COM_Compress with // comments", len > 0);
    expect_true("COM_Compress // - key1 present", std::strstr(src, "key1") != NULL);
    expect_true("COM_Compress // - key2 present", std::strstr(src, "key2") != NULL);
    expect_true("COM_Compress // - comment stripped", std::strstr(src, "comment") == NULL);

    std::strcpy(src, "a /* comment */ b");
    len = COM_Compress(src);
    expect_true("COM_Compress /* */ - a present", std::strstr(src, "a") != NULL);
    expect_true("COM_Compress /* */ - b present", std::strstr(src, "b") != NULL);
    expect_true("COM_Compress /* */ - comment stripped", std::strstr(src, "comment") == NULL);

    std::strcpy(src, "key \"quoted value\"");
    len = COM_Compress(src);
    expect_true("COM_Compress quoted - key present", std::strstr(src, "key") != NULL);
    expect_true("COM_Compress quoted - value preserved", std::strstr(src, "quoted value") != NULL);

    std::strcpy(src, "");
    len = COM_Compress(src);
    expect_int_eq("COM_Compress empty", len, 0);

    std::strcpy(src, "// just a comment\n/* another */");
    len = COM_Compress(src);
    expect_true("COM_Compress only comments", len >= 0);
}

static void test_bounds_acceptance()
{
    vec3_t mins = {1.0f, 2.0f, 3.0f};
    vec3_t maxs = {4.0f, 5.0f, 6.0f};

    vec3_t mins2 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs2 = {2.0f, 3.0f, 4.0f};
    expect_true("BoundsIntersect overlapping", BoundsIntersect(mins, maxs, mins2, maxs2) != 0);

    vec3_t mins3 = {10.0f, 10.0f, 10.0f};
    vec3_t maxs3 = {20.0f, 20.0f, 20.0f};
    expect_true("BoundsIntersect non-overlapping", BoundsIntersect(mins, maxs, mins3, maxs3) == 0);

    vec3_t origin = {2.5f, 3.5f, 4.5f};
    expect_true("BoundsIntersectSphere inside", BoundsIntersectSphere(mins, maxs, origin, 5.0f) != 0);
    expect_true("BoundsIntersectSphere far away", BoundsIntersectSphere(mins, maxs, origin, 0.1f) == 0);

    vec3_t inside = {2.0f, 3.0f, 4.0f};
    vec3_t outside = {10.0f, 10.0f, 10.0f};
    expect_true("BoundsIntersectPoint inside", BoundsIntersectPoint(mins, maxs, inside) != 0);
    expect_true("BoundsIntersectPoint outside", BoundsIntersectPoint(mins, maxs, outside) == 0);

    float rad = RadiusFromBounds(mins, maxs);
    expect_true("RadiusFromBounds positive", rad > 0.0f);

    ClearBounds(mins, maxs);
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    vec3_t c = {-1.0f, 10.0f, 0.0f};
    vec3_t d = {-5.0f, 7.0f, 2.0f};
    BoundsAdd(mins, maxs, a, b);
    BoundsAdd(mins, maxs, c, d);
    expect_float_close("BoundsAdd mins[0]", mins[0], -5.0f, 0.0001f);
    expect_float_close("BoundsAdd maxs[0]", maxs[0], 4.0f, 0.0001f);
    expect_float_close("BoundsAdd maxs[1]", maxs[1], 10.0f, 0.0001f);
}

static void test_vector_advanced_acceptance()
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    vec3_t c;
    CrossProduct(a, b, c);

    float dot_ca = DotProduct(c, a);
    expect_float_close("CrossProduct dot with a", dot_ca, 0.0f, 0.0001f);

    float dot_cb = DotProduct(c, b);
    expect_float_close("CrossProduct dot with b", dot_cb, 0.0f, 0.0001f);

    vec3_t v = {1.0f, 0.0f, 0.0f};
    vec3_t out;
    float len = VectorNormalize2(v, out);
    expect_float_close("VectorNormalize2 unit", len, 1.0f, 0.0001f);
    expect_float_close("VectorNormalize2 unit x", out[0], 1.0f, 0.0001f);
    expect_float_close("VectorNormalize2 unit y", out[1], 0.0f, 0.0001f);
    expect_float_close("VectorNormalize2 unit z", out[2], 0.0f, 0.0001f);

    v[0] = -0.5f; v[1] = 0.5f; v[2] = 1.5f;
    VectorPackTo01(v);
    expect_float_close("VectorPackTo01 x", v[0], 0.0f, 0.0001f);
    expect_float_close("VectorPackTo01 y", v[1], 0.5f, 0.0001f);
    expect_float_close("VectorPackTo01 z", v[2], 1.0f, 0.0001f);

    vec4_t v4 = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4_t out4;
    Vector4Scale(v4, 2.0f, out4);
    expect_float_close("Vector4Scale x", out4[0], 2.0f, 0.0001f);
    expect_float_close("Vector4Scale y", out4[1], 4.0f, 0.0001f);
    expect_float_close("Vector4Scale z", out4[2], 6.0f, 0.0001f);
    expect_float_close("Vector4Scale w", out4[3], 8.0f, 0.0001f);

    expect_int_eq("NearestPowerOfTwo 5", NearestPowerOfTwo(5), 8);
    expect_int_eq("NearestPowerOfTwo 8", NearestPowerOfTwo(8), 8);
    expect_int_eq("NearestPowerOfTwo 100", NearestPowerOfTwo(100), 128);
    expect_int_eq("NearestPowerOfTwo 1", NearestPowerOfTwo(1), 1);

    expect_int_eq("Q_log2 1", Q_log2(1), 0);
    expect_int_eq("Q_log2 2", Q_log2(2), 1);
    expect_int_eq("Q_log2 8", Q_log2(8), 3);
    expect_int_eq("Q_log2 16", Q_log2(16), 4);
}

static void test_plane_acceptance()
{
    vec3_t a = {0.0f, 0.0f, 0.0f};
    vec3_t b = {1.0f, 0.0f, 0.0f};
    vec3_t c = {0.0f, 1.0f, 0.0f};
    vec4_t plane;
    qboolean result = PlaneFromPoints(plane, a, b, c);
    expect_true("PlaneFromPoints XY plane", result != 0);
    expect_float_close("PlaneFromPoints normal x", plane[0], 0.0f, 0.0001f);
    expect_float_close("PlaneFromPoints normal y", plane[1], 0.0f, 0.0001f);
    expect_float_close("PlaneFromPoints normal z", plane[2], 1.0f, 0.0001f);
    expect_float_close("PlaneFromPoints dist", plane[3], 0.0f, 0.0001f);

    vec3_t a2 = {0.0f, 0.0f, 0.0f};
    vec3_t b2 = {1.0f, 1.0f, 0.0f};
    vec3_t c2 = {2.0f, 2.0f, 0.0f};
    qboolean bad = PlaneFromPoints(plane, a2, b2, c2);
    expect_true("PlaneFromPoints collinear fails", bad == 0);

    vec4_t xy_plane = {0.0f, 0.0f, 1.0f, 0.0f};
    vec3_t ray_origin = {0.0f, 0.0f, 5.0f};
    vec3_t ray_dir = {0.0f, 0.0f, -1.0f};
    vec3_t hit;
    PlaneIntersectRay(ray_origin, ray_dir, xy_plane, hit);
    expect_float_close("PlaneIntersectRay z", hit[2], 0.0f, 0.0001f);
    expect_float_close("PlaneIntersectRay x", hit[0], 0.0f, 0.0001f);
    expect_float_close("PlaneIntersectRay y", hit[1], 0.0f, 0.0001f);
}

static void test_parse_matrix_acceptance()
{
    char data[256];

    std::strcpy(data, "{ 1.0 2.0 3.0 }");
    char *buf_p = data;
    float m[3];
    Parse1DMatrix(&buf_p, 3, m);
    expect_float_close("Parse1DMatrix[0]", m[0], 1.0f, 0.0001f);
    expect_float_close("Parse1DMatrix[1]", m[1], 2.0f, 0.0001f);
    expect_float_close("Parse1DMatrix[2]", m[2], 3.0f, 0.0001f);

    std::strcpy(data, "{ { 1.0 2.0 } { 3.0 4.0 } }");
    buf_p = data;
    float m2d[2][2];
    Parse2DMatrix(&buf_p, 2, 2, (float *)m2d);
    expect_float_close("Parse2DMatrix[0][0]", m2d[0][0], 1.0f, 0.0001f);
    expect_float_close("Parse2DMatrix[0][1]", m2d[0][1], 2.0f, 0.0001f);
    expect_float_close("Parse2DMatrix[1][0]", m2d[1][0], 3.0f, 0.0001f);
    expect_float_close("Parse2DMatrix[1][1]", m2d[1][1], 4.0f, 0.0001f);

    std::strcpy(data, "{ 5.0 6.0 7.0 }");
    buf_p = data;
    float m3[3];
    Com_Parse1DMatrix(&buf_p, 3, m3, qtrue);
    expect_float_close("Com_Parse1DMatrix[0]", m3[0], 5.0f, 0.0001f);
}

static void test_growlist_acceptance()
{
    growList_t list;
    int i;

    Com_InitGrowList(&list, 2);

    int values[20];
    for (i = 0; i < 20; i++) {
        values[i] = i * 10;
        int idx = Com_AddToGrowList(&list, &values[i]);
        expect_int_eq("GrowList auto-growth index", idx, i);
    }

    for (i = 0; i < 20; i++) {
        int *ptr = (int *)Com_GrowListElement(&list, i);
        expect_true("GrowList element valid", ptr != NULL);
        if (ptr) {
            expect_int_eq("GrowList element value", *ptr, i * 10);
        }
    }

    void *ptr = Com_GrowListElement(&list, 30);
    expect_true("GrowList out-of-bounds NULL", ptr == NULL);

    for (i = 0; i < 20; i++) {
        int idx = Com_IndexForGrowListElement(&list, &values[i]);
        expect_int_eq("GrowList IndexFor each", idx, i);
    }

    Com_DestroyGrowList(&list);
}

static void test_ptr_swap_acceptance()
{
    short s_in = 0x1234;

    // ShortSwapPtr swaps bytes unconditionally
    short s_out = ShortSwapPtr(&s_in);
    expect_int_eq("ShortSwapPtr swaps bytes", (int)s_out, (int)ShortSwap(s_in));

    // FloatSwapPtr swaps bytes unconditionally
    float f_in = 1.0f;
    float f_out = FloatSwapPtr(&f_in);
    float f_back = FloatSwapPtr(&f_out);
    expect_float_close("FloatSwapPtr round-trip", f_back, f_in, 0.0001f);
}

static void test_com_sprintf_acceptance()
{
    char buf[128];

    Com_sprintf(buf, sizeof(buf), "%s %d %.2f", "test", 42, 3.14f);
    expect_str_eq("Com_sprintf basic", buf, "test 42 3.14");

    Com_sprintf(buf, 8, "this is a very long string that will be truncated");
    expect_int_eq("Com_sprintf truncated length", (int)std::strlen(buf), 7);

    const char *va_result = va("hello %d world", 99);
    expect_true("va returns non-null", va_result != NULL);
    expect_str_eq("va format", va_result, "hello 99 world");
}

static void test_crc_acceptance()
{
    unsigned char d1[] = {0x01, 0x02};
    unsigned char d2[] = {0x01, 0x02, 0x03};
    expect_true("CRC different lengths differ", CRC_Block(d1, 2) != CRC_Block(d2, 3));

    unsigned char zeros[256];
    std::memset(zeros, 0, sizeof(zeros));
    unsigned short crc_zeros = CRC_Block(zeros, 256);
    expect_true("CRC of zeros not zero", crc_zeros != 0);

    unsigned char ff[256];
    std::memset(ff, 0xFF, sizeof(ff));
    unsigned short crc_ff = CRC_Block(ff, 256);
    expect_true("CRC of 0xFF differs from zeros", crc_ff != crc_zeros);

    unsigned short crc1, crc2;
    CRC_Init(&crc1);
    CRC_ProcessByte(&crc1, 0xAB);
    CRC_ProcessByte(&crc1, 0xCD);
    CRC_ProcessByte(&crc1, 0xEF);

    unsigned char block[] = {0xAB, 0xCD, 0xEF};
    crc2 = CRC_Block(block, 3);
    expect_int_eq("CRC ProcessByte vs Block", (int)CRC_Value(crc1), (int)crc2);
}

int main()
{
    failures = 0;

    std::printf("test_compress_acceptance...\n");
    test_compress_acceptance();

    std::printf("test_bounds_acceptance...\n");
    test_bounds_acceptance();

    std::printf("test_vector_advanced_acceptance...\n");
    test_vector_advanced_acceptance();

    std::printf("test_plane_acceptance...\n");
    test_plane_acceptance();

    std::printf("test_parse_matrix_acceptance...\n");
    test_parse_matrix_acceptance();

    std::printf("test_growlist_acceptance...\n");
    test_growlist_acceptance();

    std::printf("test_ptr_swap_acceptance...\n");
    test_ptr_swap_acceptance();

    std::printf("test_com_sprintf_acceptance...\n");
    test_com_sprintf_acceptance();

    std::printf("test_crc_acceptance...\n");
    test_crc_acceptance();

    if (failures) {
        std::fprintf(stderr, "%d acceptance test(s) failed\n", failures);
        return 1;
    }

    std::printf("acceptance tests passed\n");
    return 0;
}
