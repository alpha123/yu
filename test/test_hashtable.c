#include "test.h"

#define inthash_1(x) ((u64)(x))
#define inthash_2(x) ((u64)((x)*(x)))
#define int_eq(a,b) ((a) == (b))

YU_HASHTABLE(ht, u32, char *, inthash_1, inthash_2, int_eq)
YU_HASHTABLE_IMPL(ht, u32, char *, inthash_1, inthash_2, int_eq)

#define SETUP \
    ht tbl; \
    ht_init(&tbl,3);

#define TEARDOWN \
    ht_free(&tbl);

#define LIST_HASHTABLE_TESTS(X) \
    X(getnull, "Retrieving a key that does not exists should return false") \
    X(putget, "Storing a value should allow it to be retrieved later") \
    X(putexists, "Overwriting a key should return the previous value") \
    X(getdefault, "When given a default value, get should return it if the key was not found") \
    X(grow, "Inserting more keys than the initial size should grow the table") \
    X(collide, "Collisions should be resolved, possibly by growing the table") \
    X(remove, "Removing a key should remove it from the table") \
    X(iter, "Iterating should go through all keys and values") \
    X(stopiter, "Returning non-zero should stop iteration")

TEST(getnull)
    char *s = "I shouldn't change";
    PT_ASSERT(!ht_get(&tbl, 7, &s));
    PT_ASSERT_STR_EQ(s, "I shouldn't change");
END(getnull)

TEST(putget)
    char *s = "don't touch me";
    PT_ASSERT(!ht_put(&tbl, 1, "one", &s));
    PT_ASSERT_STR_EQ(s, "don't touch me");
    PT_ASSERT(ht_get(&tbl, 1, &s));
    PT_ASSERT_STR_EQ(s, "one");
END(putget)

TEST(putexists)
    char *s;
    ht_put(&tbl, 10, "10", NULL);
    PT_ASSERT(ht_put(&tbl, 10, "ten", &s));
    PT_ASSERT_STR_EQ(s, "10");
    PT_ASSERT(ht_get(&tbl, 10, &s));
    PT_ASSERT_STR_EQ(s, "ten");
END(putexists)

TEST(getdefault)
    PT_ASSERT_STR_EQ(ht_try_get(&tbl, 2, "NUTHIN'"), "NUTHIN'");
    ht_put(&tbl, 7, "seven", NULL);
    PT_ASSERT_STR_EQ(ht_try_get(&tbl, 7, "NUTHIN'"), "seven");
END(getdefault)

char *as_words[] = {
    "zero","one","two","three","four","five",
    "six","seven","eight","nine","ten",
    "eleven","twelve","thirteen","fourteen","fifteen",
    "sixteen","seventeen","eighteen","nineteen","twenty",
    "twenty-one","twenty-two","twenty-three","twenty-four","twenty-five",
    "twenty-six","twenty-seven","twenty-eight","twenty-nine","thirty"
};

TEST(grow)
    for (u32 i = 0; i < 30; i++)
        PT_ASSERT(!ht_put(&tbl, i, as_words[i], NULL));
    PT_ASSERT_EQ(tbl.size, 30);
    for (u32 i = 0; i < 30; i++) {
        char *s = ht_try_get(&tbl, i, "missing value");
        PT_ASSERT_STR_EQ(s, as_words[i]);
    }
END(grow)

TEST(collide)
    // Note that a tbl.capacity of N means
    //   N * 2 (left+right buckets) * 2 (k/v pairs per bucket)
    // = 4N k/v pairs can be stored.
END(collide)

TEST(remove)
    for (u32 i = 0; i < 10; i++)
        ht_put(&tbl, i, as_words[i], NULL);
    PT_ASSERT(!ht_remove(&tbl, 20, NULL));
    PT_ASSERT_EQ(tbl.size, 10);
    for (u32 i = 0; i < 10; i++) {
        char *s;
        PT_ASSERT(ht_remove(&tbl, i, &s));
        PT_ASSERT_STR_EQ(s, as_words[i]);
        PT_ASSERT_EQ(tbl.size, 10 - i - 1);
    }
    PT_ASSERT_EQ(tbl.size, 0);
END(remove)

u32 iterator(u32 key, char *val, void *count) {
    PT_ASSERT_STR_EQ(val, as_words[key]);
    ++(*((u32 *)count));
    return 0;
}

TEST(iter)
    u32 n = 0;
    for (u32 i = 0; i < 10; i++)
        ht_put(&tbl, i, as_words[i], NULL);
    PT_ASSERT_EQ(ht_iter(&tbl, iterator, &n), 0);
    PT_ASSERT_EQ(n, 10);
END(iter)

u32 stop_iterator(u32 key, char * YU_UNUSED(val), void * YU_UNUSED(data)) {
    return key == 8 ? 3 : 0;
}

TEST(stopiter)
    for (u32 i = 0; i < 10; i++)
        ht_put(&tbl, i, as_words[i], NULL);
    PT_ASSERT_EQ(ht_iter(&tbl, stop_iterator, NULL), 3);
END(stopiter)

SUITE(hashtable, LIST_HASHTABLE_TESTS)
