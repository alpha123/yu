/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

struct intpair {
    int x, y;
};

s32 intpair_cmp(struct intpair a, struct intpair b) {
    return a.x - b.x;
}

YU_SPLAYTREE(st, struct intpair, intpair_cmp, true)
YU_SPLAYTREE_IMPL(st, struct intpair, intpair_cmp, true)

#define SETUP \
    st tree; \
    st_init(&tree);

#define TEARDOWN \
    st_free(&tree);

#define LIST_SPLAYTREE_TESTS(X) \
    X(insert, "Inserting a value should set it to the root") \
    X(find, "Inserted values should be retrievable") \
    X(insert_overwrite, "Inserting a value that exists should overwrite it")

TEST(insert)
    struct intpair a = {1, 10}, b = {2, 20};
    PT_ASSERT(!st_insert(&tree, a, NULL));
    PT_ASSERT_EQ(tree.root->dat.x, 1);

    PT_ASSERT(!st_insert(&tree, b, NULL));
    PT_ASSERT_EQ(tree.root->dat.x, 2);

    for (u32 i = 0; i < (u32)1e5; i++) {
	a.x = i;
	a.y = i*2;
	st_insert(&tree, a, NULL);
    }
    for (u32 i = 0; i < (u32)1e5; i++) {
	a.x = i;
	if (!st_find(&tree, a, &b)) {
	    PT_ASSERT(false);
	    break;
	}
    }
    PT_ASSERT_EQ(tree.root->dat.x, 1e5-1);
END(insert)

TEST(find)
    struct intpair a = {1, 10}, b = {1, 0}, c = {2, 20}, out;
    st_insert(&tree, a, NULL);
    st_insert(&tree, c, NULL);
    PT_ASSERT(st_find(&tree, b, &out));
    PT_ASSERT_EQ(out.y, 10);
    PT_ASSERT(st_find(&tree, c, &out));
    PT_ASSERT_EQ(out.y, 20);
END(find)

TEST(insert_overwrite)
    struct intpair a, b = {7, 700}, c = {7, 0}, out;
    for (int i = 0; i < 20; i++) {
	a.x = i;
	a.y = i * 10;
	st_insert(&tree, a, NULL);
    }
    PT_ASSERT(st_insert(&tree, b, &out));
    PT_ASSERT_EQ(out.x, 7);
    PT_ASSERT_EQ(out.y, 70);
    PT_ASSERT(st_find(&tree, c, &out));
    PT_ASSERT_EQ(out.x, 7);
    PT_ASSERT_EQ(out.y, 700);
END(insert_overwrite)


SUITE(splaytree, LIST_SPLAYTREE_TESTS)
