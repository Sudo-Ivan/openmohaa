#include "../md5.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int failures;

static void expect_str_eq(const char *name, const char *actual, const char *expected)
{
    if (std::strcmp(actual, expected) != 0) {
        std::fprintf(stderr, "FAIL: %s: expected \"%s\", got \"%s\"\n", name, expected, actual);
        failures++;
    }
}

static void hexdigest(md5_state_t *state, char *out, size_t outsize)
{
    md5_byte_t digest[16];
    md5_finish(state, digest);
    for (int i = 0; i < 16; i++) {
        std::snprintf(out + i * 2, outsize - i * 2, "%02x", digest[i]);
    }
}

static void test_md5_deterministic()
{
    md5_state_t state;
    char hash1[64], hash2[64];
    const char *input = "Test";

    // First call
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)input, std::strlen(input));
    hexdigest(&state, hash1, sizeof(hash1));

    // Second call (should match)
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)input, std::strlen(input));
    hexdigest(&state, hash2, sizeof(hash2));

    expect_str_eq("md5 deterministic", hash2, hash1);

    std::printf("  md5(\"%s\") = %s\n", input, hash1);
}

static void test_md5_known_vectors()
{
    md5_state_t state;
    char hash[64];

    struct {
        const char *input;
        const char *expected;
    } vectors[] = {
        {"", "d41d8cd98f00b204e9800998ecf8427e"},
        {"a", "0cc175b9c0f1b6a831c399e269772661"},
        {"abc", "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
        {"Test", "0cbc6611f5540bd0809a388dc95a615b"},
    };

    for (auto &v : vectors) {
        md5_init(&state);
        md5_append(&state, (const md5_byte_t *)v.input, std::strlen(v.input));
        hexdigest(&state, hash, sizeof(hash));
        expect_str_eq(v.input, hash, v.expected);
    }
}

static void test_md5_stack_garbage()
{
    md5_state_t state;
    char hash1[64], hash2[64];
    const char *input = "Test";

    // Fill stack with garbage to try to pollute state
    volatile char garbage[sizeof(md5_state_t) + 64];
    std::memset((char *)garbage, 0xFF, sizeof(garbage));

    // Compute MD5 with potentially polluted state area
    md5_state_t *pstate = (md5_state_t *)(garbage + 32);
    std::memcpy(pstate, &state, sizeof(md5_state_t)); // copy whatever is there
    // Re-init should overwrite everything needed
    md5_init(pstate);
    md5_append(pstate, (const md5_byte_t *)input, std::strlen(input));
    hexdigest(pstate, hash1, sizeof(hash1));

    // Clean run
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)input, std::strlen(input));
    hexdigest(&state, hash2, sizeof(hash2));

    expect_str_eq("md5 with garbage on stack", hash1, hash2);
}

int main()
{
    failures = 0;

    std::printf("test_md5_deterministic...\n");
    test_md5_deterministic();

    std::printf("test_md5_known_vectors...\n");
    test_md5_known_vectors();

    std::printf("test_md5_stack_garbage...\n");
    test_md5_stack_garbage();

    if (failures) {
        std::fprintf(stderr, "%d md5 test(s) failed\n", failures);
        return 1;
    }

    std::printf("md5 tests passed\n");
    return 0;
}
