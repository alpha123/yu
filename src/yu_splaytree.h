#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_splaytree.h directly! Include yu_common.h instead."
#endif

#define YU_SPLAYTREE(spl, data_t, cmp, splay_on_find) \
struct YU_NAME(spl, node) { \
    struct YU_NAME(spl, node) *left, *right; \
    data_t dat; \
}; \
\
struct YU_NAME(spl, nodelist) { \
    struct YU_NAME(spl, node) *n; \
    struct YU_NAME(spl, nodelist) *next; \
}; \
\
typedef struct { \
    struct YU_NAME(spl, node) *root; \
    struct YU_NAME(spl, nodelist) *nodes; \
    u64 size; \
} spl; \
\
void YU_NAME(spl, init)(spl *tree); \
void YU_NAME(spl, free)(spl *tree); \
\
void YU_NAME(spl, _left_rotate_)(spl *tree, struct YU_NAME(spl, node) *n); \
void YU_NAME(spl, _right_rotate_)(spl *tree, struct YU_NAME(spl, node) *n); \
struct YU_NAME(spl, node) *YU_NAME(spl, _splay_)(struct YU_NAME(spl, node) *n, data_t pt); \
\
bool YU_NAME(spl, find)(spl *tree, data_t target, data_t *out); \
bool YU_NAME(spl, insert)(spl *tree, data_t val, data_t *out); \
bool YU_NAME(spl, remove)(spl *tree, data_t val, data_t *out); \
\
bool YU_NAME(spl, min)(spl *tree, data_t *out); \
bool YU_NAME(spl, max)(spl *tree, data_t *out); \
bool YU_NAME(spl, closest)(spl *tree, data_t target, data_t *out);

#define YU_SPLAYTREE_IMPL(spl, data_t, cmp, splay_on_find) \
void YU_NAME(spl, init)(spl *tree) { \
    tree->size = 0; \
    tree->nodes = NULL; \
    tree->root = NULL; \
} \
\
void YU_NAME(spl, free)(spl *tree) { \
    struct YU_NAME(spl, nodelist) *list = tree->nodes, *next; \
\
    while (list) { \
        next = list->next; \
        free(list->n); \
        free(list); \
        list = next; \
    } \
} \
\
void YU_NAME(spl, _nodefree_)(spl *tree, struct YU_NAME(spl, node) *n) { \
    struct YU_NAME(spl, nodelist) *list = tree->nodes, *prev = NULL, *next; \
\
    while (list) { \
        next = list->next; \
        if (list->n == n) { \
            if (prev) prev->next = next; \
            else tree->nodes = next; \
            free(n); \
            free(list); \
            list = next; \
        } \
        prev = list; \
        list = next; \
    } \
} \
\
struct YU_NAME(spl, node) *YU_NAME(spl, _splay_)(struct YU_NAME(spl, node) *n, data_t pt) { \
    s32 cmp_res; \
    if (n == NULL) return n; \
    /* Construct a ‘fake’ node nn to splay around. */ \
    struct YU_NAME(spl, node) nn, *left, *right, *rot; \
    nn.left = nn.right = NULL; \
    left = right = &nn; \
\
    while (true) { \
        cmp_res = cmp(pt, n->dat); \
        if (cmp_res < 0) { \
            if (n->left == NULL) break; \
            if (cmp(pt, n->left->dat) < 0) { \
                rot = n->left; \
                n->left = rot->right; \
                rot->right = n; \
                n = rot; \
                if (n->left == NULL) break; \
            } \
            right->left = n; \
            right = n; \
            n = n->left; \
        } \
        else if (cmp_res > 0) { \
            if (n->right == NULL) break; \
            if (cmp(pt, n->right->dat) > 0) { \
                rot = n->right; \
                n->right = rot->left; \
                rot->left = n; \
                n = rot; \
                if (n->right == NULL) break; \
            } \
            left->right = n; \
            left = n; \
            n = n->right; \
        } \
        else break; \
    } \
    left->right = n->left; \
    right->left = n->right; \
    n->left = nn.right; \
    n->right = nn.left; \
    return n; \
} \
\
YU_INLINE \
struct YU_NAME(spl, node) *YU_NAME(spl, _submin_)(struct YU_NAME(spl, node) *n) { \
    while (n->left) n = n->left; \
    return n; \
} \
YU_INLINE \
struct YU_NAME(spl, node) *YU_NAME(spl, _submax_)(struct YU_NAME(spl, node) *n) { \
    while (n->right) n = n->right; \
    return n; \
} \
\
bool YU_NAME(spl, find)(spl *tree, data_t target, data_t *out) { \
    struct YU_NAME(spl, node) *n = tree->root; \
    s32 cmp_res; \
    while (n) { \
        cmp_res = cmp(target, n->dat); \
        if (cmp_res < 0) \
            n = n->left; \
        else if (cmp_res > 0) \
            n = n->right; \
        else { \
            if (out) *out = n->dat; \
            if (splay_on_find) \
                tree->root = YU_NAME(spl, _splay_)(tree->root, target); \
            return true; \
        } \
    } \
    return false; \
} \
\
bool YU_NAME(spl, insert)(spl *tree, data_t val, data_t *out) { \
    struct YU_NAME(spl, node) *n = tree->root, *nn = NULL; \
    struct YU_NAME(spl, nodelist) *nln; \
    s32 cmp_res; \
\
    if (n == NULL) { \
        ++tree->size; \
        nn = calloc(1, sizeof(struct YU_NAME(spl, node))); \
        nln = calloc(1, sizeof(struct YU_NAME(spl, nodelist))); \
        nln->n = nn; \
        nln->next = tree->nodes; \
        tree->nodes = nln; \
        nn->dat = val; \
        tree->root = nn; \
        return false; \
    } \
\
    n = tree->root = YU_NAME(spl, _splay_)(n, val); \
    cmp_res = cmp(val, n->dat); \
    if (cmp_res == 0) { \
        /* replace n->dat in case cmp only compares on part of a data_t (e.g. for maps) */ \
        if (out) *out = n->dat; \
        n->dat = val; \
        return true; \
    } \
    ++tree->size; \
    nn = calloc(1, sizeof(struct YU_NAME(spl, node))); \
    nln = calloc(1, sizeof(struct YU_NAME(spl, nodelist))); \
    nln->n = nn; \
    nln->next = tree->nodes; \
    tree->nodes = nln; \
    nn->dat = val; \
\
    if (cmp_res < 0) { \
        nn->left = n->left; \
        nn->right = n; \
        n->left = NULL; \
    } \
    else { \
        nn->right = n->right; \
        nn->left = n; \
        n->right = NULL; \
    } \
    tree->root = nn; \
    return false; \
} \
\
bool YU_NAME(spl, remove)(spl *tree, data_t val, data_t *out) { \
    struct YU_NAME(spl, node) *n; \
    if (tree->root == NULL) return false; \
    n = tree->root = YU_NAME(spl, _splay_)(tree->root, val); \
    if (cmp(val, n->dat) == 0) { \
        if (n->left == NULL) \
            tree->root = n->right; \
        else { \
            tree->root = YU_NAME(spl, _splay_)(n->left, val); \
            tree->root->right = n->right; \
        } \
        if (out) *out = n->dat; \
        YU_NAME(spl, _nodefree_)(tree, n); \
        --tree->size; \
        return true; \
    } \
    return false; \
} \
\
bool YU_NAME(spl, min)(spl *tree, data_t *out) { \
    assert(out != NULL); \
    if (tree->root == NULL) \
        return false; \
    *out = YU_NAME(spl, _submin_)(tree->root)->dat; \
    if (splay_on_find) tree->root = YU_NAME(spl, _splay_)(tree->root, *out); \
    return true; \
} \
\
bool YU_NAME(spl, max)(spl *tree, data_t *out) { \
    assert(out != NULL); \
    if (tree->root == NULL) \
        return false; \
    *out = YU_NAME(spl, _submax_)(tree->root)->dat; \
    if (splay_on_find) tree->root = YU_NAME(spl, _splay_)(tree->root, *out); \
    return true; \
} \
\
bool YU_NAME(spl, closest)(spl *tree, data_t target, data_t *out) { \
    assert(out != NULL); \
    struct YU_NAME(spl, node) *n = tree->root, *last; \
    int cmp_res; \
    if (n == NULL) \
        return false; \
    while (n) { \
        last = n; \
        cmp_res = cmp(target, n->dat); \
        if (cmp_res < 0) \
            n = n->left; \
        else if (cmp_res > 0) \
            n = n->right; \
        else { \
            last = n; \
            break; \
        } \
    } \
    *out = last->dat; \
    if (splay_on_find) tree->root = YU_NAME(spl, _splay_)(tree->root, *out); \
    return true; \
}
