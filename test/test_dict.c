/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "dict.h"

#define SETUP \
    yu_memctx_t mctx; \
    TEST_GET_ALLOCATOR(&mctx);

#define TEARDOWN \
    yu_alloc_ctx_free(&mctx);

#define LIST_DICT_TESTS(X) \
    X(perfect_hash, "Perfect hash should have no collisions")

#ifdef TEST_FAST
const size_t WORD_COUNT = 8000;
#else
const size_t WORD_COUNT = 79339;
#endif

TEST(perfect_hash)
    phf_string_t ss[] = {{.p = "foo", .n = 3}, {.p = "bar", .n = 3}};
    struct phf p;
    phf_init(&p, ss, 2, 0, 77, 314, false, &mctx);
    u32 h1 = phf_hash(&p, ss[0]),
        h2 = phf_hash(&p, ss[1]),
        h3 = phf_hash(&p, ss[0]);
    PT_ASSERT_EQ(h1, h3);
    PT_ASSERT_NEQ(h1, h2);
    phf_free(&p);

    FILE *wfile = fopen("test/words.txt", "r");
    assert(wfile != NULL);

    phf_string_t *words = yu_xalloc(&mctx, WORD_COUNT, sizeof(phf_string_t));
    for (u32 i = 0; i < WORD_COUNT; i++) {
        char *s = yu_xalloc(&mctx, 100, 1);
        char *ok = fgets(s, 100, wfile);
        assert(ok);
        words[i].p = s;
        words[i].n = strlen(s);
    }

    bool *hashes = yu_xalloc(&mctx, WORD_COUNT+1000, sizeof(bool));
    phf_init(&p, words, WORD_COUNT, 0, WORD_COUNT+1000, 59991, false, &mctx);

    bool no_collisions = true;
    for (u32 i = 0; i < WORD_COUNT; i++) {
        if (hashes[phf_hash(&p, words[i])]) {
            no_collisions = false;
            break;
        }
        hashes[phf_hash(&p, words[i])] = true;
    }
    PT_ASSERT(no_collisions);
END(perfect_hash)


SUITE(dict, LIST_DICT_TESTS)
