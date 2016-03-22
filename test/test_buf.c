#include "test.h"

#include "sys_alloc.h"

#define SETUP \
    yu_buf_ctx ctx; \
    TEST_GET_ALLOCATOR(mctx); \
    yu_buf_ctx_init(&ctx, (yu_allocator *)&mctx);

#define TEARDOWN \
    yu_buf_ctx_free(&ctx); \
    yu_alloc_ctx_free(&mctx);

#define LIST_BUF_TESTS(X) \
    X(intern, "Frozen buffers should be interned") \
    X(non_interned, "Non-frozen buffers should be allocated individually") \
    X(cat, "Concatenating buffers should return a buffer with concatenated contents") \
    X(cat_interned, "Interned concatenation should intern the resulting buffer") \
    X(make_interned, "Performing operations on a non-interned buffer and then freezing it should intern it") \
    X(userdata, "Buffers should be able to store associated data")

#define new_buf(str, intern) yu_buf_new(&ctx, (unsigned char *)(str), strlen((str)), (intern))

TEST(intern)
    yu_buf a, b;
    a = new_buf("foo", true);
    b = new_buf("foo", true);
    PT_ASSERT(a == b);
END(intern)

TEST(non_interned)
    yu_buf a, b;
    a = new_buf("bar", false);
    b = new_buf("bar", false);
    PT_ASSERT(a != b);
    yu_buf_free(b);
    b = new_buf("bar", true);
    PT_ASSERT(a != b);
    yu_buf_free(a);
    a = new_buf("bar", true);
    PT_ASSERT(a == b);
END(non_interned)

TEST(cat)
    yu_buf a, b, c;
    a = new_buf("the quick brown fox", false);
    b = new_buf(" jumps over the lazy dog", false);
    c = yu_buf_cat(a, b, false);
    PT_ASSERT_EQ(yu_buf_len(c), 43u);
    PT_ASSERT_EQ(memcmp(c, "the quick brown fox jumps over the lazy dog", 43), 0);
END(cat)

TEST(cat_interned)
    yu_buf a, b, c, d;
    a = new_buf("the quick brown fox", true);
    b = new_buf(" jumps over the lazy dog", true);
    c = new_buf("the quick brown fox jumps over the lazy dog", true);
    d = yu_buf_cat(a, b, true);
    PT_ASSERT(c == d);
    yu_buf_free(c);
    c = new_buf("the quick brown fox jumps over the lazy dog", true);
    PT_ASSERT(c == d);
    yu_buf_free(a);
    yu_buf_free(b);
    yu_buf_free(c);
    yu_buf_free(d);

    a = new_buf("foo", false);
    b = new_buf("bar", false);
    c = yu_buf_cat(a, b, true);
    PT_ASSERT_EQ(memcmp(c, "foobar", 6), 0);
    a[2] = 'x';
    PT_ASSERT_EQ(memcmp(c, "foobar", 6), 0);
    d = yu_buf_cat(a, b, true);
    PT_ASSERT_EQ(memcmp(d, "foxbar", 6), 0);
    PT_ASSERT(c != d);
    yu_buf_free(a);
    a = new_buf("foxbar", true);
    PT_ASSERT(a == d);
END(cat_interned)

TEST(make_interned)
    yu_buf a, b, c, d;
    a = new_buf("silver soul", true);
    b = new_buf("silver", false);
    c = new_buf(" soul", false);
    d = yu_buf_cat(b, c, false);
    yu_buf_free(b);
    yu_buf_free(c);
    PT_ASSERT(a != d);
    PT_ASSERT(yu_buf_len(a) == yu_buf_len(d));
    PT_ASSERT_EQ(memcmp(a, d, 11), 0);
    b = yu_buf_freeze(d);
    PT_ASSERT(a == b);
    PT_ASSERT(b != d);
    yu_buf_free(d);
    PT_ASSERT_EQ(memcmp(b, "silver soul", 11), 0);
END(make_interned)

TEST(userdata)
    int n = 42;
    yu_buf a, b;
    a = new_buf("gin", false);
    b = new_buf("tama", true);
    yu_buf_set_udata(a, &n);
    yu_buf_set_udata(b, a);
    PT_ASSERT_EQ(*(int *)yu_buf_get_udata(yu_buf_get_udata(b)), 42);
END(userdata)

SUITE(buf, LIST_BUF_TESTS)
