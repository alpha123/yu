/*#include "yu_common.h"

#include <stdio.h>

#define STR(x) #x

int main(int argc, char **argv) {
    u64 fnv = yu_fnv1a((void *)argv[1], strlen(argv[1]));
    u64 m2 = yu_murmur2((void *)argv[1], strlen(argv[1]));
    printf("%llx, %llx\n", fnv, m2);
    return 0;
}*/

#include "yu_common.h"

#include <inttypes.h>
#include <stdio.h>

u64 inthash1(u64 n) { return n; }
u64 inthash2(u64 n) { return n + 1; }
bool inteq(u64 a, u64 b) { return a == b; }

YU_HASHTABLE(int_hash, u64, u64, inthash1, inthash2, inteq)
YU_HASHTABLE_IMPL(int_hash, u64, u64, inthash1, inthash2, inteq)

u32 print_table(u64 k, u64 v, void *data) {
    printf("%" PRIu64 ": %" PRIu64 "\n", k, v);
    return 0;
}

u32 check_table(u64 k, u64 v, void *data) {
    if (k * 10 == v)
        return 0;
    return (u32)k;
}

int main(int argc, char **argv) {
    u64 n;
    int_hash t;

    int_hash_init(&t, 3);
    int_hash_put(&t, 1, 10, &n);
    int_hash_put(&t, 2, 20, &n);
    int_hash_put(&t, 3, 30, &n);
    int_hash_put(&t, 4, 40, &n);
    int_hash_put(&t, 5, 50, &n);
    int_hash_put(&t, 6, 60, &n);
    int_hash_put(&t, 7, 70, &n);
    int_hash_put(&t, 8, 80, &n);
    int_hash_put(&t, 10, 100, &n);
    int_hash_put(&t, 20, 200, &n);
    int_hash_put(&t, 30, 300, &n);
    int_hash_put(&t, 40, 400, &n);
    int_hash_put(&t, 50, 500, &n);
    int_hash_put(&t, 60, 600, &n);
    int_hash_put(&t, 70, 700, &n);
    int_hash_put(&t, 80, 800, &n);

    int_hash_get(&t, 3, &n);
    printf("3 * 10: %" PRIu64 "\n", n);

    int_hash_get(&t, 40, &n);
    printf("40 * 10: %" PRIu64 "\n", n);

    if (int_hash_get(&t, 90, &n))
        printf("90 * 10: %" PRIu64 "\n", n);
    else
        printf("90 * 10: no idea\n");

    int_hash_put(&t, 90, 1000, &n);
    if (int_hash_get(&t, 90, &n))
        printf("90 * 10: %" PRIu64 "\n", n);
    else
        printf("90 * 10: no idea\n");

    if (int_hash_put(&t, 90, 900, &n))
        printf("oops! was %" PRIu64 "\n", n);
    if (int_hash_get(&t, 90, &n))
        printf("90 * 10: %" PRIu64 "\n", n);
    else
        printf("90 * 10: no idea\n");

    if (int_hash_remove(&t, 90, &n))
        printf("heh, gone now. was %" PRIu64 "\n", n);
    if (int_hash_get(&t, 90, &n))
        printf("90 * 10: %" PRIu64 "\n", n);
    else
        printf("90 * 10: no idea\n");

    if (int_hash_put(&t, 90, 900, &n))
        printf("oops! was %" PRIu64 "\n", n);
    if (int_hash_get(&t, 90, &n))
        printf("90 * 10: %" PRIu64 "\n", n);
    else
        printf("90 * 10: no idea\n");

    int_hash_iter(&t, print_table, NULL);

    printf("size: %" PRIu64 ", k: %" PRIu32 "\n", t.size, t.capacity);

    for (u32 i = 0; i < 10000; ++i)
        int_hash_put(&t, i, i * 10, &n);

    printf("size: %" PRIu64 ", k: %" PRIu32 "\n", t.size, t.capacity);

    if ((n = int_hash_iter(&t, check_table, NULL)))
        printf("sanity check: FAIL on %" PRIu64 "\n", n);
    else
        printf("sanity check: PASS\n");

    return 0;
}
