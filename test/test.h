#ifndef TEST_H_
#define TEST_H_

#define TEST_USE_COLOR 1

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test_results_
{
    char status;
    char name[128];
    char error[2048];
    struct
    {
        uint32_t pass, fail, skip, todo, unknown;
    } stats;
    struct
    {
        const char* file;
        int line;
    } context;
};

// meh, for testing a global is fine IMO
static struct test_results_ TEST_results;

#define TEST_TOSTRING_(x)   #x
#define TEST_TOSTRING(x)    TEST_TOSTRING_(x)

#if TEST_USE_COLOR
#define TEST_COLOR(c)   "\033[" TEST_TOSTRING(c) "m"
#else
#define TEST_COLOR(c)   ""
#endif
#define TEST_PRINT_ERROR_(...)  snprintf(TEST_results.error, sizeof(TEST_results.error), __VA_ARGS__)

static void test_dumpstr_escaped_(char* buf, const char* str, int len)
{
    if(!str)
    {
        memcpy(buf, "<null>", sizeof("<null>"));
        return;
    }
    if(len < 0) len = strlen(str);
    *buf++ = '`';
    while(len--)
    {
        uint8_t c = *str++;
        switch(c)
        {
        case '\r': *buf++ = '\\'; *buf++ = 'r'; break;
        case '\n': *buf++ = '\\'; *buf++ = 'n'; break;
        case '\t': *buf++ = '\\'; *buf++ = 't'; break;
        case '\v': *buf++ = '\\'; *buf++ = 'v'; break;
        case '\f': *buf++ = '\\'; *buf++ = 'f'; break;
        case '\\': *buf++ = '\\'; *buf++ = '\\'; break;
        case '`': *buf++ = '\\'; *buf++ = '`'; break;
        default:
            if(c < ' ' || 0x80 <= c)
            {
                snprintf(buf, 5, "\\x%.2X", c);
                buf += 4;
            }
            else
                *buf++ = c;
            break;
        }
    }
    *buf++ = '`';
    *buf = 0;
}
static char* test_dumpstr_escaped_alloc_(const char* str, int len)
{
    if(len < 0) len = str ? strlen(str) : sizeof("<null>") - 1;
    char* buf = malloc(4 * len + 7);
    test_dumpstr_escaped_(buf, str, len);
    return buf;
}
static void test_handle_result_(void)
{
    // ensure test results are flushed first
    fflush(stdout);
    fflush(stderr);
    const char* bcolor;
    const char* dcolor;
    const char* word;
    if(TEST_results.context.file || TEST_results.context.line)
        printf("%s:%d: error: " TEST_COLOR(0), TEST_results.context.file ? TEST_results.context.file : "?", TEST_results.context.line);
    switch(TEST_results.status)
    {
    case 'P': bcolor = TEST_COLOR(92); dcolor = TEST_COLOR(32); word = "PASS"; ++TEST_results.stats.pass; break;
    case 'S': bcolor = TEST_COLOR(96); dcolor = TEST_COLOR(36); word = "SKIP"; ++TEST_results.stats.skip; break;
    case 'T': bcolor = TEST_COLOR(93); dcolor = TEST_COLOR(33); word = "TODO"; ++TEST_results.stats.todo; break;
    case 'F': bcolor = TEST_COLOR(91); dcolor = TEST_COLOR(31); word = "FAIL"; ++TEST_results.stats.fail; break;
    default: bcolor = dcolor = ""; word = "??" "??"; ++TEST_results.stats.unknown; break;
    }
    printf("%s%s%s: %s" TEST_COLOR(0), bcolor, word, dcolor, TEST_results.name);
    putchar('\n');
    if(TEST_results.error[0])
    {
        size_t i;
        for(i = 0; TEST_results.error[i]; i++)
        {
            if(!i || TEST_results.error[i-1] == '\n')
                fputs(">\t", stdout);
            putchar(TEST_results.error[i]);
        }
        if(TEST_results.error[i-1] != '\n')
            putchar('\n');
    }
    fflush(stdout);
    fflush(stderr);
}
static int test_report_(void)
{
    printf("----------\n");
    uint32_t all = TEST_results.stats.pass + TEST_results.stats.skip + TEST_results.stats.todo + TEST_results.stats.fail + TEST_results.stats.unknown;
    if (TEST_results.stats.pass > 0)
        printf(TEST_COLOR(92) "TOTAL PASS" TEST_COLOR(32) ": %" PRIu32 TEST_COLOR(90) "/%" PRIu32 TEST_COLOR(0) "\n", TEST_results.stats.pass, all);
    if (TEST_results.stats.skip > 0)
    printf(TEST_COLOR(96) "TOTAL SKIP" TEST_COLOR(36) ": %" PRIu32 TEST_COLOR(90) "/%" PRIu32 TEST_COLOR(0) "\n", TEST_results.stats.skip, all);
    if (TEST_results.stats.todo > 0)
    printf(TEST_COLOR(93) "TOTAL TODO" TEST_COLOR(33) ": %" PRIu32 TEST_COLOR(90) "/%" PRIu32 TEST_COLOR(0) "\n", TEST_results.stats.todo, all);
    if (TEST_results.stats.fail > 0)
    printf(TEST_COLOR(91) "TOTAL FAIL" TEST_COLOR(31) ": %" PRIu32 TEST_COLOR(90) "/%" PRIu32 TEST_COLOR(0) "\n", TEST_results.stats.fail, all);
    if(TEST_results.stats.unknown)
        printf(TEST_COLOR(97) "*** WARNING: Had %" PRIu32 " unknown test%s. Possible memory corruption or invalid API use?" TEST_COLOR(0) "\n", TEST_results.stats.unknown, TEST_results.stats.unknown >= 2 ? "s" : "");
    printf("Executed " TEST_COLOR(97) "%" PRIu32 TEST_COLOR(0) " tests.\n", all);
    return TEST_results.stats.fail + TEST_results.stats.unknown;
}

#define TEST_RESULT_(STATUS, FILE, LINE, ...)   do { TEST_results.status = (STATUS); TEST_PRINT_ERROR_(__VA_ARGS__); TEST_results.context.file = FILE; TEST_results.context.line = LINE; } while(0)
#define TEST_RETURN_(STATUS, ...)   do { TEST_RESULT_(STATUS, __FILE__, __LINE__, __VA_ARGS__); return; } while(0)
#ifdef TEST_TRAP_ON_FAIL
#define TEST_HELPER_(HELPER, ...)   do { if(!HELPER(__VA_ARGS__, __FILE__, __LINE__)) __builtin_trap(); } while(0)
#else
#define TEST_HELPER_(HELPER, ...)   do { if(!HELPER(__VA_ARGS__, __FILE__, __LINE__)) return; } while(0)
#endif

#define TEST(NAME)              static void test_##NAME()
#define TEST_EXEC(NAME)                                                 \
    do {                                                                \
        TEST_results.status = 'P';                                      \
        strncpy(TEST_results.name, #NAME, sizeof(TEST_results.name));   \
        TEST_results.name[sizeof(TEST_results.name)-1] = 0;             \
        TEST_results.error[0] = 0;                                      \
        TEST_results.context.file = NULL;                               \
        TEST_results.context.line = 0;                                  \
        test_##NAME();                                                  \
        test_handle_result_();                                          \
    } while(0)
#define TEST_SKIP(NAME, ...)                                            \
    do {                                                                \
        strncpy(TEST_results.name, #NAME, sizeof(TEST_results.name));   \
        TEST_results.name[sizeof(TEST_results.name)-1] = 0;             \
        TEST_RESULT_('S', __FILE__, __LINE__, __VA_ARGS__);             \
        (void)test_##NAME;                                              \
        test_handle_result_();                                          \
    } while(0)
#define TEST_REPORT()       test_report_()

bool test_assert_eq_ptr_(const void* x, const void* y, const char* msg, const char* file, int line)
{
    if(x == y)
        return true;
    TEST_RESULT_('F', file, line, "%s [[%p == %p]]", msg, x, y);
    return false;
}
bool test_assert_eq_str_(const char* x, const char* y, const char* msg, const char* file, int line)
{
    if(x == y || (x && y && !strcmp(x, y)))
        return true;
    char* bx = test_dumpstr_escaped_alloc_(x, -1);
    char* by = test_dumpstr_escaped_alloc_(y, -1);
    TEST_RESULT_('F', file, line, "%s [[%s == %s]]", msg, bx, by);
    free(bx);
    free(by);
    return false;
}
bool test_assert_eq_int_(int64_t x, int64_t y, const char* msg, const char* file, int line)
{
    if(x == y)
        return true;
    TEST_RESULT_('F', file, line, "%s [[%" PRIi64 " == %" PRIi64 "]]", msg, x, y);
    return false;
}
bool test_assert_ne_int_(int64_t x, int64_t y, const char* msg, const char* file, int line)
{
    if(x != y)
        return true;
    TEST_RESULT_('F', file, line, "%s [[%" PRIi64 " != %" PRIi64 "]]", msg, x, y);
    return false;
}
bool test_assert_eq_uint_(uint64_t x, uint64_t y, const char* msg, const char* file, int line)
{
    if(x == y)
        return true;
    TEST_RESULT_('F', file, line, "%s [[%" PRIu64 " == %" PRIu64 "]]", msg, x, y);
    return false;
}

// PRECONDITION() is used to check the test harness itself (e.g. whether memory allocation failed)
#define PRECONDITION(x)     do { if(!(x)) TEST_RETURN_('F', "Precondition failed: %s", #x); } while(0)

#define ASSERT_TODO(...)    TEST_RETURN_('T', __VA_ARGS__)
#define ASSERT_MSG(x, ...)  do { if(!(x)) TEST_RETURN_('F', __VA_ARGS__); } while(0)
#define ASSERT(x)           ASSERT_MSG(x, #x)

#define ASSERT_EQ_PTR(x, y) TEST_HELPER_(test_assert_eq_ptr_, x, y, #x " == " #y)
#define ASSERT_EQ_STR(x, y) TEST_HELPER_(test_assert_eq_str_, x, y, #x " == " #y)
#define ASSERT_EQ_INT(x, y) TEST_HELPER_(test_assert_eq_int_, x, y, #x " == " #y)
#define ASSERT_NE_INT(x, y) TEST_HELPER_(test_assert_ne_int_, x, y, #x " == " #y)
#define ASSERT_EQ_UINT(x, y) TEST_HELPER_(test_assert_eq_uint_, x, y, #x " == " #y)


#endif /* TEST_H_ */
