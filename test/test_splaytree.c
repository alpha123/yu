/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "internal_alloc.h"

struct intpair {
    int x, y;
};

s32 intpair_cmp(struct intpair a, struct intpair b) {
    return a.x - b.x;
}

YU_SPLAYTREE(st, struct intpair, intpair_cmp, true)
YU_SPLAYTREE_IMPL(st, struct intpair, intpair_cmp, true)

#define SETUP \
    yu_memctx_t mctx; \
    st tree; \
    internal_alloc_ctx_init(&mctx); \
    st_init(&tree, &mctx);

#define TEARDOWN \
    st_free(&tree); \
    yu_alloc_ctx_free(&mctx);

#define LIST_SPLAYTREE_TESTS(X) \
    X(insert, "Inserting a value should set it to the root") \
    X(find, "Inserted values should be retrievable") \
    X(find_set_root, "Finding a value should update the root node") \
    X(insert_overwrite, "Inserting a value that exists should overwrite it") \
    X(remove, "Removing a value should remove it from the tree and adjust the root") \
    X(min, "Min should return the smallest value in the tree") \
    X(max, "Max should return the largest value in the tree") \
    X(closest, "Closest should find the value nearest to its argument")

TEST(insert)
    struct intpair a = {1, 10}, b = {2, 20};
    PT_ASSERT(!st_insert(&tree, a, NULL));
    PT_ASSERT_EQ(tree.root->dat.x, 1);

    PT_ASSERT(!st_insert(&tree, b, NULL));
    PT_ASSERT_EQ(tree.root->dat.x, 2);
END(insert)

TEST(find)
    struct intpair a = {1, 10}, b = {1, 0}, c = {2, 20}, out = {-1, -1};
    st_insert(&tree, a, NULL);
    st_insert(&tree, c, NULL);
    PT_ASSERT(st_find(&tree, b, &out));
    PT_ASSERT_EQ(out.y, 10);
    PT_ASSERT(st_find(&tree, c, &out));
    PT_ASSERT_EQ(out.y, 20);

#ifdef TEST_FAST
    for (u32 i = 0; i < 40; i++)
#else
    for (u32 i = 0; i < (u32)3e6; i++)
#endif
    {
        a.x = i;
        a.y = i*2;
        st_insert(&tree, a, NULL);
    }

#ifdef TEST_FAST
    for (u32 i = 0; i < 40; i++)
#else
    for (u32 i = 0; i < (u32)3e6; i++)
#endif
    {
        a.x = i;
        if (!st_find(&tree, a, &b)) {
            PT_ASSERT(false);
            break;
        }
    }
#ifdef TEST_FAST
    PT_ASSERT_EQ(tree.root->dat.x, 39);
#else
    PT_ASSERT_EQ(tree.root->dat.x, 3e6-1);
#endif
END(find)

TEST(find_set_root)
    struct intpair a;
    for (int i = 0; i < 20; i++) {
        a.x = i;
        a.y = i * 10;
        st_insert(&tree, a, NULL);
    }
    a.x = 4;
    st_find(&tree, a, NULL);
    PT_ASSERT_EQ(tree.root->dat.y, 40);
    a.x = 7;
    st_find(&tree, a, NULL);
    PT_ASSERT_EQ(tree.root->dat.y, 70);
    a.x = 3;
    st_find(&tree, a, NULL);
    PT_ASSERT_EQ(tree.root->dat.y, 30);
END(find_set_root)

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

TEST(remove)
    struct intpair a, b, c = {100, 1000};
    for (int i = 0; i < 20; i++) {
        a.x = i;
        a.y = i * 10;
        st_insert(&tree, a, NULL);
    }
    b.x = 40;
    PT_ASSERT(!st_remove(&tree, b, &c));
    // Shouldn't touch the out parameter if the needle isn't found
    PT_ASSERT_EQ(c.x, 100);
    b.x = 3;
    PT_ASSERT(st_remove(&tree, b, &c));
    PT_ASSERT_EQ(c.x, 3);
    PT_ASSERT_EQ(c.y, 30);
    // Root will be the last one we searched
    PT_ASSERT_EQ(tree.root->dat.x, 2);
    PT_ASSERT(!st_find(&tree, b, &c));
    PT_ASSERT(!st_insert(&tree, c, NULL));
END(remove)

TEST(min)
    struct intpair a, b;
    PT_ASSERT(!st_min(&tree, &b));

    for (int i = 5; i <= 300; i += 5) {
        a.x = i;
        a.y = i * 10;
        st_insert(&tree, a, NULL);
    }
    PT_ASSERT(st_min(&tree, &b));
    PT_ASSERT_EQ(b.x, 5);
    PT_ASSERT_EQ(b.y, 50);
    PT_ASSERT_EQ(tree.root->dat.x, 5);
END(min)

TEST(max)
    struct intpair a, b;
    PT_ASSERT(!st_max(&tree, &b));

    for (int i = 5; i <= 300; i += 5) {
        a.x = i;
        a.y = i * 10;
        st_insert(&tree, a, NULL);
    }
    PT_ASSERT(st_max(&tree, &b));
    PT_ASSERT_EQ(b.x, 300);
    PT_ASSERT_EQ(b.y, 3000);
    PT_ASSERT_EQ(tree.root->dat.x, 300);
END(max)

TEST(closest)
    struct intpair a, b, c, d;
    b.x = 50;
    PT_ASSERT(!st_closest(&tree, b, &c, &d));

    for (int i = 5; i <= 300; i += 5) {
        a.x = i;
        a.y = i * 10;
        st_insert(&tree, a, NULL);
    }

    b.x = 50;
    PT_ASSERT(st_closest(&tree, b, &c, &d));
    PT_ASSERT_EQ(c.y, 500);
    PT_ASSERT_EQ(d.y, 500);
    PT_ASSERT_EQ(tree.root->dat.x, c.x);

    b.x = 42;
    st_closest(&tree, b, &c, &d);
    PT_ASSERT_EQ(c.x, 40);
    PT_ASSERT_EQ(d.x, 45);
    PT_ASSERT_EQ(tree.root->dat.x, c.x);

    b.x = 87;
    st_closest(&tree, b, &c, &d);
    PT_ASSERT_EQ(c.x, 85);
    PT_ASSERT_EQ(d.x, 90);
    PT_ASSERT_EQ(tree.root->dat.x, c.x);

    b.x = 0;
    st_closest(&tree, b, &c, &d);
    PT_ASSERT_EQ(c.x, 5);
    PT_ASSERT_EQ(d.x, 5);
    PT_ASSERT_EQ(tree.root->dat.x, c.x);

    b.x = 1000;
    st_closest(&tree, b, &c, &d);
    PT_ASSERT_EQ(c.x, 300);
    PT_ASSERT_EQ(d.x, 300);
    PT_ASSERT_EQ(tree.root->dat.x, c.x);
END(closest)


SUITE(splaytree, LIST_SPLAYTREE_TESTS)
