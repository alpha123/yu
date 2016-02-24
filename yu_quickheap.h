#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_quickheap.h directly! Include yu_common.h instead."
#endif

/**
 * A very fast, cache-friendly priority queue based on Incremental Quicksort.
 * See http://www.dcc.uchile.cl/TR/2008/TR_DCC-2008-006.pdf for details
 * This is a rather rough implementation almost directly from the paper.
 */

#define YU_QUICKHEAP_MINHEAP 0
#define YU_QUICKHEAP_MAXHEAP 1

#define YU_QUICKHEAP(qh, data_t, cmp, is_maxheap) \
typedef struct { \
    u64 size, \
        fc_idx, /* first cell index */ \
        psc, /* pivot stack counter */ \
        *pivstack; /* stack of pivot indices */ \
    data_t *elems; \
    u8 cap, /* size of elems is 2^cap */ \
       pivstack_cap; \
} qh; \
\
void YU_NAME(qh, init)(qh *heap, u64 size); \
void YU_NAME(qh, free)(qh *heap); \
data_t YU_NAME(qh, top)(qh *heap, data_t ifempty); \
data_t YU_NAME(qh, pop)(qh *heap, data_t ifempty); \
void YU_NAME(qh, push)(qh *heap, data_t val);

#define YU_QUICKHEAP_IMPL(qh, data_t, cmp, is_maxheap) \
void YU_NAME(qh, init)(qh *heap, u64 size) { \
    YU_ERR_DEFVAR \
    heap->size = 0; \
    heap->psc = 1; \
    heap->fc_idx = 0; \
    heap->pivstack_cap = 4; \
    YU_CHECK_ALLOC(heap->pivstack = calloc(1 << heap->pivstack_cap, sizeof(u64))); \
    heap->cap = yu_ceil_log2(size + 1); \
    YU_CHECK_ALLOC(heap->elems = calloc(1 << heap->cap, sizeof(data_t))); \
    YU_ERR_DEFAULT_HANDLER(NOTHING()) \
} \
\
void YU_NAME(qh, free)(qh *heap) { \
    free(heap->pivstack); \
    free(heap->elems); \
} \
\
YU_INLINE \
s32 YU_NAME(qh, cmp)(data_t a, data_t b) { \
    if (is_maxheap) \
        return cmp(b, a); \
    return cmp(a, b); \
} \
\
YU_INLINE \
data_t YU_NAME(qh, elem)(qh *heap, u64 i) { \
    return heap->elems[i & (1 << heap->cap) - 1]; \
} \
\
void YU_NAME(qh, setelem)(qh *heap, u64 i, data_t val) { \
    YU_ERR_DEFVAR \
    data_t *stay_safe; \
    while (i >= 1 << heap->cap) { \
        stay_safe = heap->elems; \
        YU_CHECK_ALLOC(heap->elems = realloc(heap->elems, sizeof(data_t) * (1 << ++heap->cap))); \
    } \
    heap->elems[i] = val; \
\
    if (yu_local_err == YU_ERR_ALLOC_FAIL) \
        heap->elems = stay_safe; \
    YU_ERR_DEFAULT_HANDLER(NOTHING()) \
} \
\
u64 YU_NAME(qh, paritition)(data_t *elems, data_t pivot, u64 lo, u64 hi) { \
    u64 swap_idx = lo; \
    data_t swap; \
\
    while (lo <= hi) { \
        while (YU_NAME(qh, cmp)(pivot, elems[lo]) > 0) \
            ++lo; \
        while (YU_NAME(qh, cmp)(pivot, elems[hi]) < 0) \
            --hi; \
        if (lo < hi) { \
            swap = elems[lo]; \
            elems[lo] = elems[hi]; \
            elems[hi] = swap; \
            ++lo; \
            --hi; \
        } \
        else if (lo == hi) \
            break; \
    } \
\
    return lo; \
} \
\
void YU_NAME(qh, _iqs_)(data_t *elems, u64 idx, u64 **ps, u64 *ps_sz, u8 *ps_cap) { \
    YU_ERR_DEFVAR \
    u64 *s = *ps, z = *ps_sz - 1, pidx, qidx, *safety_first; \
    while (idx != s[z]) { \
        pidx = idx + (s[z]-1 - idx) / 2; \
        qidx = YU_NAME(qh, paritition)(elems, elems[pidx], idx, s[z]-1); \
        if (*ps_sz == 1 << *ps_cap) { \
            safety_first = *ps; \
            YU_CHECK_ALLOC(*ps = realloc(*ps, 1 << ++(*ps_cap))); \
            s = *ps; \
        } \
        s[(*ps_sz)++] = qidx; \
        ++z; \
    } \
    /* If the size is 2, avoid decrementing the pivot stack twice (it gets decremented on pop) \
       Truth be told, I'm not totally sure why this works, but keep the faux pivot s[0] in place. */ \
    if (*ps_sz > 2) --(*ps_sz); \
\
    if (yu_local_err == YU_ERR_ALLOC_FAIL) \
        *ps = safety_first; \
    YU_ERR_DEFAULT_HANDLER(NOTHING()) \
} \
\
YU_INLINE \
void YU_NAME(qh, _iqs_heap_)(qh *heap) { \
    YU_NAME(qh, _iqs_)(heap->elems, heap->fc_idx, &heap->pivstack, &heap->psc, &heap->pivstack_cap); \
} \
\
data_t YU_NAME(qh, top)(qh *heap, data_t ifempty) { \
    if (heap->size == 0) \
        return ifempty; \
    YU_NAME(qh, _iqs_heap_)(heap); \
    return YU_NAME(qh, elem)(heap, heap->fc_idx); \
} \
\
data_t YU_NAME(qh, pop)(qh *heap, data_t ifempty) { \
    if (heap->size == 0) \
        return ifempty; \
    YU_NAME(qh, _iqs_heap_)(heap); \
    ++heap->fc_idx; \
    --heap->psc; \
    --heap->size; \
    return YU_NAME(qh, elem)(heap, heap->fc_idx-1); \
} \
\
void YU_NAME(qh, push)(qh *heap, data_t val) { \
    u64 pidx = 0; \
    while (true) { \
        YU_NAME(qh, setelem)(heap, heap->pivstack[pidx] + 1, YU_NAME(qh, elem)(heap, heap->pivstack[pidx])); \
        ++heap->pivstack[pidx]; \
        if (pidx + 1 >= heap->psc || YU_NAME(qh, cmp)(YU_NAME(qh, elem)(heap, heap->pivstack[pidx + 1]), val) <= 0) { \
            YU_NAME(qh, setelem)(heap, heap->pivstack[pidx] - 1, val); \
            ++heap->size; \
            return; \
        } \
        YU_NAME(qh, setelem)(heap, heap->pivstack[pidx] - 1, YU_NAME(qh, elem)(heap, heap->pivstack[pidx + 1] + 1)); \
        ++pidx; \
    } \
}
