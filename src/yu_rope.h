#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_rope.h directly! Include yu_common.h instead."
#endif

typedef enum { yu_rope_leaf, yu_rope_cat } yu_rope_node_t;

typedef struct yu_rope {
    yu_rope_node_t what;
    struct yu_rope *parent;
    union {
        struct {
            struct yu_rope *left;
            struct yu_rope *right;
        } cat;
        yu_str leaf;
    } node;
} * yu_rope;

yu_rope yu_rope_new(yu_str s);
yu_rope yu_rope_cat(yu_rope a, yu_rope b);
yu_rope yu_rope_subrope(yu_rope r, u64 start, u64 len);
yu_str yu_rope_tostr(yu_rope r);
