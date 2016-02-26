#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_numeric.h directly! Include yu_common.h instead."
#endif

/* Yu has the following numeric types:
 *   - fixnum: s32, but limited to 16 bits so we can promote to num instead of overflowing
 *   - num: mpfr mpfr_t
 *   - rat: gmp mpq_t
 *   - vec: openblas single precision vector
 *   - mat: openblas single precision matrix (column-major)
 */

#include <gmp.h>
#include <mpfr.h>
#include <cblas.h>

typedef s32 yu_fixnum;
typedef mpfr_t yu_real;
typedef mpq_t yu_rat;
typedef float * yu_vec;
typedef float * yu_mat;

typedef enum {
    YU_NUM_FIXNUM, YU_NUM_REAL, YU_NUM_RAT,
    YU_NUM_VEC, YU_NUM_MAT
} yu_num_t;

typedef struct {
    yu_num_t what;
    union {
        yu_fixnum i;
        yu_real r;
        yu_rat q;
        yu_vec v;
        yu_mat m;
    } n;
} yu_num;

struct yu_mat_dat {
    s64 rows;
    s64 cols;
    void *base;
};

#define YU_MAT_DAT(m) ((struct yu_mat_dat *)(m) - 1)

// These accept s64 because that's what BLAS uses for specifying
// array lengths.
// They will fail if given a negative number.
yu_vec yu_vec_new(s64 len);
yu_mat yu_mat_new(s64 cols, s64 rows);

// Creates a fixnum for 16-bit ints and a real otherwise
void yu_mkint(s32 n, yu_num *out);
void yu_mkreal(mpfr_t n, yu_num *out);
// If the denominator is one and the numerator fits in 16 bits,
// make a fixnum.
void yu_mkrat(mpq_t n, yu_num *out);
void yu_mkvec(yu_vec v, yu_num *out);
void yu_mkmat(yu_mat m, yu_num *out);

YU_INLINE
void yu_vec_free(yu_vec v) {
    free(YU_MAT_DAT(v)->base);
}
#define yu_mat_free(m) yu_vec_free((yu_vec)(m))

YU_INLINE
s64 yu_mat_cols(yu_mat m) {
    return YU_MAT_DAT(m)->cols;
}

YU_INLINE
s64 yu_mat_rows(yu_mat m) {
    return YU_MAT_DAT(m)->rows;
}
#define yu_vec_len(v) yu_mat_rows((yu_mat)(v))

YU_ERR_RET yu_num_add(yu_num *a, yu_num *b, yu_num *out);
