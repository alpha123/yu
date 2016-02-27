#include "yu_common.h"
#include <stdio.h>

#include "SFMT/SFMT.h"

#define key_t int
#define val_t int
#define sl sl
#define cmp INT_CMP
#define INT_CMP(a,b) ((a) - (b))
#define maxheight 33
#define binsize 4
#define seed 1234

#include "yu_skiplist.h"

s32 iter(int k, int v, void *_data) {
    printf("%d * 10 = %d\n", k, v);
    return 0;
}

int main(int argc, char **argv) {
    sl list;
    sl_init(&list);
    int n;

    if (sl_insert(&list, 1, 5, &n))
        printf("overwrote 1: %d\n", n);
    if (sl_find(&list, 1, &n))
        printf("1: %d\n", n);
    else
        puts("didn't find 1");

    if (sl_insert(&list, 2, 20, &n))
        printf("overwrote 2: %d\n", n);
    if (sl_find(&list, 2, &n))
        printf("2: %d\n", n);
    else
        puts("didn't find 2");

    if (sl_insert(&list, 5, 50, &n))
        printf("overwrote 5: %d\n", n);
    if (sl_find(&list, 5, &n))
        printf("5: %d\n", n);
    else
        puts("didn't find 5");

    if (sl_insert(&list, 3, 30, &n))
        printf("overwrote 3: %d\n", n);
    if (sl_find(&list, 3, &n))
        printf("3: %d\n", n);
    else
        puts("didn't find 3");

    if (sl_insert(&list, 4, 40, &n))
        printf("overwrote 4: %d\n", n);
    if (sl_find(&list, 4, &n))
        printf("4: %d\n", n);
    else
        puts("didn't find 4");

    if (sl_insert(&list, 1, 7, &n))
        printf("overwrote 1: %d\n", n);
    if (sl_find(&list, 1, &n))
        printf("1: %d\n", n);
    else
        puts("didn't find 1");

    if (sl_insert(&list, 1, 10, &n))
        printf("overwrote 1: %d\n", n);
    if (sl_find(&list, 1, &n))
        printf("1: %d\n", n);
    else
        puts("didn't find 1");

    if (sl_insert(&list, 3, 30, &n))
        printf("overwrote 3: %d\n", n);
    if (sl_find(&list, 3, &n))
        printf("3: %d\n", n);
    else
        puts("didn't find 3");

    if (sl_insert(&list, 8, 80, &n))
        printf("overwrote 8: %d\n", n);
    if (sl_find(&list, 8, &n))
        printf("8: %d\n", n);
    else
        puts("didn't find 8");

    if (sl_insert(&list, 7, 70, &n))
        printf("overwrote 7: %d\n", n);
    if (sl_find(&list, 7, &n))
        printf("7: %d\n", n);
    else
        puts("didn't find 7");

    if (sl_insert(&list, 6, 60, &n))
        printf("overwrote 6: %d\n", n);
    if (sl_find(&list, 6, &n))
        printf("6: %d\n", n);
    else
        puts("didn't find 6");

    sl_iter(&list, iter, NULL);

    sl_free(&list);
    return 0;
}
