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

static void test_math_smoke()
{
    vec3_t v, out;

    v[0] = 0.0f; v[1] = 0.0f; v[2] = 0.0f;
    float len = VectorNormalize(v);
    expect_true("VectorNormalize zero returns 0", len == 0.0f);

    len = VectorNormalize2(v, out);
    expect_true("VectorNormalize2 zero returns 0", len == 0.0f);

    float rsqrt = Q_rsqrt(4.0f);
    expect_float_close("Q_rsqrt(4)", rsqrt, 0.5f, 0.001f);

    expect_int_eq("ClampByte 128", ClampByte(128), 128);
    expect_int_eq("ClampByte -1", ClampByte(-1), 0);
    expect_int_eq("ClampByte 256", ClampByte(256), 255);

    expect_int_eq("ClampChar 64", ClampChar(64), 64);
    expect_int_eq("ClampChar -129", ClampChar(-129), -128);
    expect_int_eq("ClampChar 128", ClampChar(128), 127);

    expect_float_close("AngleMod 720", AngleMod(720.0f), 0.0f, 0.0001f);
    expect_float_close("AngleMod 90", AngleMod(90.0f), 90.0f, 0.0001f);

    vec3_t axis[3];
    AxisClear(axis);
    expect_float_close("AxisClear[0][0]", axis[0][0], 0.0f, 0.0001f);

    vec3_t mins, maxs;
    ClearBounds(mins, maxs);
    expect_true("ClearBounds mins > maxs", mins[0] > maxs[0]);

    v[0] = 5.0f; v[1] = -3.0f; v[2] = 10.0f;
    AddPointToBounds(v, mins, maxs);
    expect_float_close("AddPoint bounds mins[0]", mins[0], 5.0f, 0.0001f);
    expect_float_close("AddPoint bounds maxs[1]", maxs[1], -3.0f, 0.0001f);

    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    expect_float_close("DotProduct (1,2,3).(4,5,6)", DotProduct(a, b), 32.0f, 0.0001f);

    v[0] = 1.0f; v[1] = -2.0f; v[2] = 3.0f;
    VectorInverse(v);
    expect_float_close("VectorInverse x", v[0], -1.0f, 0.0001f);
    expect_float_close("VectorInverse y", v[1], 2.0f, 0.0001f);
    expect_float_close("VectorInverse z", v[2], -3.0f, 0.0001f);

    unsigned color = ColorBytes3(1.0f, 0.5f, 0.0f);
    expect_int_eq("ColorBytes3(1,0.5,0)", (int)color, 0x0000FF7F);
}

static void test_string_smoke()
{
    char buf[16];

    Q_strncpyz(buf, "test", 0);

    Q_strncpyz(buf, "test", 1);
    expect_int_eq("Q_strncpyz to 1-byte buf", (int)buf[0], 0);

    buf[0] = '\0';
    Q_strcat(buf, sizeof(buf), "hello");
    expect_str_eq("Q_strcat with zero-size buf", buf, "");

    buf[0] = '\0';
    Q_strncpyz(buf, "a", sizeof(buf));
    Q_strcat(buf, sizeof(buf), "b");
    expect_str_eq("Q_strcat", buf, "ab");

    const char *r = Q_strrchr("ababa", 'b');
    if (r) {
        expect_int_eq("Q_strrchr position", (int)(r - "ababa"), 3);
    }

    expect_true("Q_stristr finds match", Q_stristr("Hello World", "world") != NULL);
    expect_true("Q_stristr no match", Q_stristr("Hello World", "foo") == NULL);

    expect_true("Q_isprint space", Q_isprint(' ') != 0);
    expect_true("Q_islower a", Q_islower('a') != 0);
    expect_true("Q_islower A", Q_islower('A') == 0);
    expect_true("Q_isupper A", Q_isupper('A') != 0);
    expect_true("Q_isalpha a", Q_isalpha('a') != 0);
    expect_true("Q_isalpha 1", Q_isalpha('1') == 0);
    expect_true("Q_isanumber 123", Q_isanumber("123") != 0);
    expect_true("Q_isanumber abc", Q_isanumber("abc") == 0);
    expect_true("Q_isintegral 1.0", Q_isintegral(1.0) != 0);
    expect_true("Q_isintegral 1.5", Q_isintegral(1.5) == 0);

    expect_int_eq("Q_PrintStrlen basic", (int)Q_PrintStrlen("hello"), 5);
    expect_int_eq("Q_CountChar", Q_CountChar("abracadabra", 'a'), 5);

    char path[] = "main\\maps\\test.bsp";
    Com_BackslashToSlash(path);
    expect_str_eq("Com_BackslashToSlash", path, "main/maps/test.bsp");

    expect_int_eq("Com_HexStrToInt FF", Com_HexStrToInt("FF"), 255);
    expect_int_eq("Com_HexStrToInt 0", Com_HexStrToInt("0"), 0);
    expect_int_eq("Com_HexStrToInt 10", Com_HexStrToInt("10"), 16);

    char lwr[] = "Hello World";
    Q_strlwr(lwr);
    expect_str_eq("Q_strlwr", lwr, "hello world");
    Q_strupr(lwr);
    expect_str_eq("Q_strupr", lwr, "HELLO WORLD");
}

static void test_path_smoke()
{
    const char *path;
    char stripped[64];

    path = COM_SkipPath("main/maps/test.bsp");
    expect_str_eq("COM_SkipPath", path, "test.bsp");

    path = COM_SkipPath("test.bsp");
    expect_str_eq("COM_SkipPath no dir", path, "test.bsp");

    const char *ext = COM_GetExtension("test.bsp");
    expect_str_eq("COM_GetExtension", ext, "bsp");

    ext = COM_GetExtension("test");
    expect_int_eq("COM_GetExtension no ext", (int)ext[0], 0);

    COM_StripExtension("test.bsp", stripped, sizeof(stripped));
    expect_str_eq("COM_StripExtension", stripped, "test");

    COM_StripExtension("test", stripped, sizeof(stripped));
    expect_str_eq("COM_StripExtension no ext", stripped, "test");

    char with_ext[64] = "test.bsp";
    COM_DefaultExtension(with_ext, sizeof(with_ext), ".bsp");
    expect_str_eq("COM_DefaultExtension already has", with_ext, "test.bsp");

    char no_ext[64] = "test";
    COM_DefaultExtension(no_ext, sizeof(no_ext), ".bsp");
    expect_str_eq("COM_DefaultExtension added", no_ext, "test.bsp");

    expect_true("COM_CompareExtension match", COM_CompareExtension("test.bsp", ".bsp") != 0);
    expect_true("COM_CompareExtension no match", COM_CompareExtension("test.bsp", ".wav") == 0);
}

static void test_parse_smoke()
{
    const char *result;

    char data[] = "hello world 42";
    char *buf_p = data;

    COM_BeginParseSession("test");
    result = COM_Parse(&buf_p);
    expect_str_eq("COM_Parse first token", result, "hello");

    result = COM_Parse(&buf_p);
    expect_str_eq("COM_Parse second token", result, "world");

    result = COM_Parse(&buf_p);
    expect_str_eq("COM_Parse third token", result, "42");

    result = COM_Parse(&buf_p);
    expect_int_eq("COM_Parse after end", (int)result[0], 0);

    char src[] = "token1\n// comment\ntoken2\n/* block */token3";
    int len = COM_Compress(src);
    expect_true("COM_Compress reduced size", len > 0 && len < (int)sizeof(src));
    expect_true("COM_Compress contains token1", std::strstr(src, "token1") != NULL);
    expect_true("COM_Compress contains token2", std::strstr(src, "token2") != NULL);
    expect_true("COM_Compress contains token3", std::strstr(src, "token3") != NULL);
}

static void test_info_smoke()
{
    char info[256] = {0};

    expect_true("Info_Validate empty", Info_Validate("") != 0);
    expect_true("Info_Validate valid", Info_Validate("\\key\\value") != 0);

    Info_SetValueForKey(info, "name", "player1");
    const char *v = Info_ValueForKey(info, "name");
    expect_str_eq("Info_ValueForKey", v, "player1");

    v = Info_ValueForKey(info, "nonexistent");
    expect_int_eq("Info_ValueForKey missing", (int)v[0], 0);

    Info_SetValueForKey(info, "name", "player2");
    v = Info_ValueForKey(info, "name");
    expect_str_eq("Info_ValueForKey updated", v, "player2");

    Info_SetValueForKey(info, "model", "soldier");
    v = Info_ValueForKey(info, "model");
    expect_str_eq("Info_ValueForKey second key", v, "soldier");

    Info_RemoveKey(info, "model");
    v = Info_ValueForKey(info, "model");
    expect_int_eq("Info_RemoveKey", (int)v[0], 0);

    Info_SetValueForKey(info, "a", "1");
    Info_SetValueForKey(info, "b", "2");
    const char *scan = info;
    char key[64], val[64];
    Info_NextPair(&scan, key, val);
    expect_true("Info_NextPair finds a key", key[0] != '\0');
}

static void test_growlist_smoke()
{
    growList_t list;

    Com_InitGrowList(&list, 4);
    int val1 = 42;
    int idx = Com_AddToGrowList(&list, &val1);
    expect_int_eq("GrowList first element index", idx, 0);

    int *ptr = (int *)Com_GrowListElement(&list, 0);
    expect_true("GrowList first element value", ptr != NULL && *ptr == 42);

    int val2 = 99;
    idx = Com_AddToGrowList(&list, &val2);
    expect_int_eq("GrowList second element index", idx, 1);

    idx = Com_IndexForGrowListElement(&list, &val1);
    expect_int_eq("GrowList index of first", idx, 0);

    idx = Com_IndexForGrowListElement(&list, &val2);
    expect_int_eq("GrowList index of second", idx, 1);

    int val3 = 0;
    idx = Com_IndexForGrowListElement(&list, &val3);
    expect_int_eq("GrowList index of unknown", idx, -1);

    Com_DestroyGrowList(&list);
}

static void test_endian_smoke()
{
    short s = ShortSwap((short)0x1234);
    expect_int_eq("ShortSwap 0x1234", (int)s, (int)(short)0x3412);

    unsigned short us = UnsignedShortSwap((unsigned short)0xABCD);
    expect_int_eq("UnsignedShortSwap 0xABCD", (int)us, (int)(unsigned short)0xCDAB);

    int l = LongSwap(0x12345678);
    expect_int_eq("LongSwap 0x12345678", l, 0x78563412);

    unsigned int ul = UnsignedLongSwap(0xDEADBEEFu);
    expect_int_eq("UnsignedLongSwap 0xDEADBEEF", (int)ul, (int)0xEFBEADDEu);

    s = (short)0xABCD;
    expect_int_eq("ShortSwap round-trip", (int)ShortSwap(ShortSwap(s)), (int)s);

    us = (unsigned short)0xDEAD;
    expect_int_eq("UnsignedShortSwap round-trip", (int)UnsignedShortSwap(UnsignedShortSwap(us)), (int)us);

    l = 0xCAFEBABE;
    expect_int_eq("LongSwap round-trip", LongSwap(LongSwap(l)), l);

    float f = 3.14159f;
    float f2 = FloatSwap(&f);
    float f3 = FloatSwap(&f2);
    expect_float_close("FloatSwap round-trip", f3, f, 0.0001f);

    // Ptr variants should behave like swap
    short sp = ShortSwapPtr(&s);
    expect_int_eq("ShortSwapPtr matches ShortSwap", (int)sp, (int)ShortSwap(s));

    int lp = LongSwapPtr(&l);
    expect_int_eq("LongSwapPtr matches LongSwap", lp, LongSwap(l));
}

static void test_crc_smoke()
{
    unsigned short crc;

    CRC_Init(&crc);
    expect_int_eq("CRC_Init value", (int)crc, 0xFFFF);

    expect_int_eq("CRC_Value after init", (int)CRC_Value(crc), 0xFFFF);

    CRC_ProcessByte(&crc, 0);
    expect_true("CRC changes after processing byte", CRC_Value(crc) != 0xFFFF);

    unsigned short empty_crc = CRC_Block((unsigned char *)"", 0);
    expect_int_eq("CRC_Block empty", (int)empty_crc, 0xFFFF);

    unsigned char buf1[] = {0x01, 0x02, 0x03};
    unsigned char buf2[] = {0x01, 0x02, 0x03};
    expect_int_eq("CRC_Block deterministic", (int)CRC_Block(buf1, 3), (int)CRC_Block(buf2, 3));
}

int main()
{
    failures = 0;

    std::printf("test_math_smoke...\n");
    test_math_smoke();

    std::printf("test_string_smoke...\n");
    test_string_smoke();

    std::printf("test_path_smoke...\n");
    test_path_smoke();

    std::printf("test_parse_smoke...\n");
    test_parse_smoke();

    std::printf("test_info_smoke...\n");
    test_info_smoke();

    std::printf("test_growlist_smoke...\n");
    test_growlist_smoke();

    std::printf("test_endian_smoke...\n");
    test_endian_smoke();

    std::printf("test_crc_smoke...\n");
    test_crc_smoke();

    if (failures) {
        std::fprintf(stderr, "%d smoke test(s) failed\n", failures);
        return 1;
    }

    std::printf("smoke tests passed\n");
    return 0;
}
