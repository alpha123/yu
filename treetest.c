#include <stdio.h>
#include "yu_common.h"

struct intpair {
    int x, y;
};
#define INTPAIR_CMP(a,b) ((a).x - (b).x)
YU_SPLAYTREE(int_tree, struct intpair, INTPAIR_CMP, true)
YU_SPLAYTREE_IMPL(int_tree, struct intpair, INTPAIR_CMP, true)

int main(int argc, char **argv) {
    int_tree t;
    int_tree_init(&t);
    struct intpair n,
      zero  = {0,  0},
      one   = {1, 10}, oneb = {1, 7},
      two   = {2, 20},
      three = {3, 30},
      four  = {4, 40},
      five  = {5, 50},
      six   = {6, 60},
      seven = {7, 70},
      eight = {8, 80},
      nine  = {9, 90},
      ten   = {10, 100},
      eleven= {11, 110},
      big   = {500, 5000},
      ns[]  = {zero,one,two,three,four,five,six,seven,eight,nine,ten,eleven};

    int_tree_insert(&t, oneb, &n);
    if (int_tree_find(&t, oneb, &n))
        printf("found 1: %d\n", n.y);
    else
        puts("not found");

    for (int i = 1; i < 11; i++) {
        if (i > 6 && i < 9) continue;
        if (int_tree_insert(&t, ns[i], &n))
            printf("overwrote %d: %d\n", n.x, n.y);
        else
            printf("inserted %d: %d\n", ns[i].x, ns[i].y);
    }
    int_tree_insert(&t, eight, &n);
     if (int_tree_remove(&t, six, &n))
         printf("removed 6: %d\n", n.y);
     else
         puts("6 wasn't found");
    int_tree_insert(&t, seven, &n);
    for (int i = 0; i < 12; i++) {
        if (int_tree_find(&t, ns[i], &n))
            printf("%d × 10 = %d\n", n.x, n.y);
        else
            printf("didn't find %d\n", ns[i].x);
    }

    int_tree_min(&t, &n);
    printf("min: %d: %d\n", n.x, n.y);
    int_tree_max(&t, &n);
    printf("max: %d: %d\n", n.x, n.y);

    int_tree_closest(&t, zero, &n);
    printf("closest to 0: %d: %d\n", n.x, n.y);
    int_tree_closest(&t, six, &n);
    printf("closest to 6: %d: %d\n", n.x, n.y);
    int_tree_closest(&t, ten, &n);
    printf("closest to 10: %d: %d\n", n.x, n.y);
    int_tree_closest(&t, big, &n);
    printf("closest to 500: %d: %d\n", n.x, n.y);

    int_tree_free(&t);
    return 0;
}
