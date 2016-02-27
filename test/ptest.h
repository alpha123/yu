#ifndef ptest_h
#define ptest_h

#include <string.h>

#define PT_SUITE(name) void name(void)

#define PT_FUNC(name) static void name(void)
#define PT_REG(name) pt_add_test(name, #name, __func__)
#define PT_TEST(name) auto void name(void); PT_REG(name); void name(void)

#define PT_PRINTF_FORMAT(x) (_Generic((x), \
      int8_t: PRIi8,	      uint8_t: PRIu8, \
     int16_t: PRIi16,	     uint16_t: PRIu16, \
     int32_t: PRIi32,	     uint32_t: PRIu32, \
     int64_t: PRIi64,	     uint64_t: PRIu64, \
       float: "g",	       double: "g", \
      char *: "s",    unsigned char *: "s", \
const char *: "s", const unsigned char *: "s", \
      void *: "p"))

#define PT_ASSERT(expr) pt_assert_run((int)(expr), #expr, __func__, __FILE__, __LINE__)
#define PT_ASSERT_EQUAL(expr, expect, eql) do{ \
    __typeof__(expr) _actual = (expr); \
    __typeof__(expect) _hope = (expect); \
    char *_fmt, *_msg; \
    asprintf(&_fmt, "%s == %s: Expected '%%%s', got '%%%s'.", \
             #expr, #expect, PT_PRINTF_FORMAT(_hope), PT_PRINTF_FORMAT(_actual)), \
    asprintf(&_msg, _fmt, _hope, _actual); \
    pt_assert_run(eql((expr),(expect)), _msg, __func__, __FILE__, __LINE__); \
    free(_fmt); \
    free(_msg); \
}while(0)
#define PT_RAWEQ(a, b) ((a) == (b))
#define PT_STREQ(a, b) (strcmp((a),(b)) == 0)
#define PT_ASSERT_EQ(expr, expect) PT_ASSERT_EQUAL((expr), (expect), PT_RAWEQ)
#define PT_ASSERT_STR_EQ(expr, expect) PT_ASSERT_EQUAL((char *)(expr), (char *)(expect), PT_STREQ)

void pt_assert_run(int result, const char* expr, const char* func, const char* file, int line);

void pt_add_test(void (*func)(void), const char* name, const char* suite);
void pt_add_suite(void (*func)(void));
int pt_run(void);

#endif
