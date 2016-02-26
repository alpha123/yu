#include <stdio.h>
#include "yu_common.h"

#define INT_CMP(a,b) ((a)-(b))

YU_QUICKHEAP(qh, int, INT_CMP, YU_QUICKHEAP_MINHEAP)
YU_QUICKHEAP_IMPL(qh, int, INT_CMP, YU_QUICKHEAP_MINHEAP)

struct word_count {
    const char *word;
    u32 count;
};

s32 wc_cmp(struct word_count *a, struct word_count *b) {
    return a->count - b->count;
}

YU_QUICKHEAP(sh, struct word_count *, wc_cmp, YU_QUICKHEAP_MAXHEAP)
YU_QUICKHEAP_IMPL(sh, struct word_count *, wc_cmp, YU_QUICKHEAP_MAXHEAP)

int main(int argc, char **argv) {
    sfmt_t rng;
    sfmt_init_gen_rand(&rng, 135135);
    qh heap;
    qh_init(&heap, 5);
    printf("pushing: ");
    for (int i = 0; i < 30; i++) {
        int n = (int)(sfmt_genrand_uint32(&rng) % 100);
        printf("%d ", n);
        qh_push(&heap, n);
    }
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("popping: ");
    while (heap.size)
        printf("%d ", qh_pop(&heap, -1));
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("pushing: ");
    for (int i = 0; i < 15; i++) {
        int n = (int)(sfmt_genrand_uint32(&rng) % 100);
        printf("%d ", n);
        qh_push(&heap, n);
    }
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("popping: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", qh_pop(&heap, -1));
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("pushing: ");
    for (int i = 0; i < 15; i++) {
        int n = (int)(sfmt_genrand_uint32(&rng) % 100);
        printf("%d ", n);
        qh_push(&heap, n);
    }
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("popping: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", qh_pop(&heap, -1));
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("pushing: ");
    for (int i = 0; i < 15; i++) {
        int n = (int)(sfmt_genrand_uint32(&rng) % 100);
        printf("%d ", n);
        qh_push(&heap, n);
    }
    printf("\nsize: %"PRIu64"\n", heap.size);
    printf("top: %d\n", qh_top(&heap, -1));
    printf("popping: ");
    while (heap.size)
        printf("%d ", qh_pop(&heap, -1));
    printf("\nsize: %"PRIu64"\n", heap.size);

    qh_free(&heap);


    sh sheap;
    sh_init(&sheap, 10);

    struct word_count words[] =
        {{"strawberry", 1}, {"milk", 1},
         {"of", 4}, {"but", 4}, {"to", 6},
         {"you", 10}, {"the", 9}, {"that", 4},
         {"too", 2}, {"it", 3}, {"get", 2},
         {0, 0}};

    for (int i = 0; words[i].word; i++)
        sh_push(&sheap, words + i);

    while (sheap.size) {
        struct word_count *wc;
        wc = sh_pop(&sheap, words + sizeof(words) / sizeof(struct word_count) - 1);
        printf("%s: %d\n", wc->word, wc->count);
    }

    sh_free(&sheap);

    return 0;
}
