#include <cstdio>
#include <cstring>
#include <cstdlib>

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

/*
 * Simulate SV_RehashBans_f parsing logic.
 * Format:
 *   0 <ip> <subnet>\n
 *   1 <ip> <subnet>:<reason>\n
 *
 * Returns number of bans parsed, or -1 on error.
 */
static int parse_bans(const char *textbuf, int filelen,
                      char (*out_ips)[64], int *out_subnets,
                      int *out_isexceptions, int maxbans)
{
    const char *curpos, *endpos, *maskpos, *newlinepos;
    int index;

    curpos = textbuf;
    endpos = textbuf + filelen;

    for (index = 0; index < maxbans && curpos + 2 < endpos; index++)
    {
        const char *reasonpos;

        // find the end of the address string
        for (maskpos = curpos + 2; maskpos < endpos && *maskpos != ' '; maskpos++);

        if (maskpos + 1 >= endpos)
            break;

        // skip the space
        maskpos++;

        // find the end of the subnet specifier
        for (newlinepos = maskpos; newlinepos < endpos && *newlinepos != '\n' && *newlinepos != ':'; newlinepos++);

        if (newlinepos >= endpos)
            break;

        if (*newlinepos == ':') {
            reasonpos = newlinepos + 1;
            for (newlinepos = reasonpos; newlinepos < endpos && *newlinepos != '\n'; newlinepos++);
            if (newlinepos >= endpos)
                break;
        }

        // Extract the IP (between curpos+2 and the space)
        int iplen = (int)(maskpos - (curpos + 2) - 1);
        if (iplen > 0 && iplen < 64) {
            std::memcpy(out_ips[index], curpos + 2, iplen);
            out_ips[index][iplen] = '\0';
        }

        // Extract subnet
        int subnet_len = (int)(newlinepos - maskpos);
        char subnet_buf[16];
        if (subnet_len > 0 && subnet_len < 16) {
            std::memcpy(subnet_buf, maskpos, subnet_len);
            subnet_buf[subnet_len] = '\0';
            out_subnets[index] = std::atoi(subnet_buf);
        }

        out_isexceptions[index] = (curpos[0] != '0');

        curpos = newlinepos + 1;
    }

    return index;
}

static void test_parse_simple_ban()
{
    const char *data = "0 192.168.1.100 32\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("simple ban count", count, 1);
    if (count >= 1) {
        expect_true("simple ban isexception==0", isexceptions[0] == 0);
        expect_int_eq("simple ban subnet", subnets[0], 32);
        expect_true("simple ban ip match", std::strcmp(ips[0], "192.168.1.100") == 0);
    }
}

static void test_parse_exception_with_reason()
{
    const char *data = "1 10.0.0.1 24:whitelisted\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("exception count", count, 1);
    if (count >= 1) {
        expect_true("exception isexception==1", isexceptions[0] == 1);
        expect_int_eq("exception subnet", subnets[0], 24);
        expect_true("exception ip match", std::strcmp(ips[0], "10.0.0.1") == 0);
    }
}

static void test_parse_multiple_bans()
{
    const char *data = "0 192.168.1.1 32\n1 10.0.0.1 8\n0 172.16.0.1 16\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("multi ban count", count, 3);
    if (count >= 3) {
        expect_true("ban1 ip", std::strcmp(ips[0], "192.168.1.1") == 0);
        expect_int_eq("ban1 subnet", subnets[0], 32);
        expect_int_eq("ban1 isexception", isexceptions[0], 0);

        expect_true("ban2 ip", std::strcmp(ips[1], "10.0.0.1") == 0);
        expect_int_eq("ban2 subnet", subnets[1], 8);
        expect_int_eq("ban2 isexception", isexceptions[1], 1);

        expect_true("ban3 ip", std::strcmp(ips[2], "172.16.0.1") == 0);
        expect_int_eq("ban3 subnet", subnets[2], 16);
    }
}

static void test_parse_with_reason_and_trailing_newlines()
{
    const char *data = "0 192.168.1.1 32:bad actor\n1 10.0.0.1 24\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("reason ban count", count, 2);
    if (count >= 1) {
        expect_true("reason ban ip", std::strcmp(ips[0], "192.168.1.1") == 0);
        expect_int_eq("reason ban subnet", subnets[0], 32);
    }
}

static void test_parse_empty_file()
{
    const char *data = "";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("empty count", count, 0);
}

static void test_parse_invalid_line()
{
    // Line with just newline - curpos+2 would be past endpos
    const char *data = "\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("invalid line count", count, 0);
}

static void test_parse_ipv6()
{
    const char *data = "0 ::1 128\n1 2001:db8::1 64\n";
    char ips[4][64];
    int subnets[4];
    int isexceptions[4];

    int count = parse_bans(data, std::strlen(data), ips, subnets, isexceptions, 4);
    expect_int_eq("ipv6 count", count, 2);
    if (count >= 2) {
        expect_int_eq("ipv6 ban1 subnet", subnets[0], 128);
        expect_int_eq("ipv6 ban2 subnet", subnets[1], 64);
        expect_int_eq("ipv6 ban1 isexception", isexceptions[0], 0);
        expect_int_eq("ipv6 ban2 isexception", isexceptions[1], 1);
    }
}

int main()
{
    failures = 0;

    std::printf("test_parse_simple_ban...\n");
    test_parse_simple_ban();

    std::printf("test_parse_exception_with_reason...\n");
    test_parse_exception_with_reason();

    std::printf("test_parse_multiple_bans...\n");
    test_parse_multiple_bans();

    std::printf("test_parse_with_reason_and_trailing_newlines...\n");
    test_parse_with_reason_and_trailing_newlines();

    std::printf("test_parse_empty_file...\n");
    test_parse_empty_file();

    std::printf("test_parse_invalid_line...\n");
    test_parse_invalid_line();

    std::printf("test_parse_ipv6...\n");
    test_parse_ipv6();

    if (failures) {
        std::fprintf(stderr, "%d ban test(s) failed\n", failures);
        return 1;
    }

    std::printf("ban tests passed\n");
    return 0;
}
