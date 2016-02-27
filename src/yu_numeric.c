#include "yu_common.h"

yu_vec yu_vec_new(s64 len) {
    return (yu_vec)yu_mat_new(1, len);
}

yu_mat yu_mat_new(s64 cols, s64 rows) {
    YU_ERR_DEFVAR
    assert(cols > 0);
    assert(rows > 0);

    void *base = malloc(sizeof(struct yu_mat_dat) + rows * cols * sizeof(float) + 15);
    YU_CHECK_ALLOC(base);
    struct yu_mat_dat *d = (struct yu_mat_dat *)((uintptr_t)base + 15 & ~(uintptr_t)0x0f);
    d->cols = cols;
    d->rows = rows;
    d->base = base;
    return (yu_mat)(d + 1);
    YU_ERR_DEFAULT_HANDLER(NULL)
}

void yu_mkint(s32 n, yu_num *out) {
    if (n >= -32768 && n <= 32767) {
        out->what = YU_NUM_FIXNUM;
        out->n.i = n;
    }
    else {
        out->what = YU_NUM_REAL;
        mpfr_init_set_si(out->n.r, n, MPFR_RNDN);
    }
}

void yu_mkreal(mpfr_t n, yu_num *out) {
    out->what = YU_NUM_REAL;
    mpfr_init_set(out->n.r, n, MPFR_RNDN);
}

void yu_mkrat(mpq_t n, yu_num *out) {
    // Still results in a mpq_t for -32768/1, but whatever.
    if (mpz_cmp_si(mpq_denref(n), 1) == 0 &&
        mpz_cmpabs_ui(mpq_numref(n), 32768) < 0)
            yu_mkint(mpz_get_si(mpq_numref(n)), out);
    else {
        out->what = YU_NUM_RAT;
        mpq_init(out->n.q);
        mpq_set(out->n.q, n);
    }
}

void yu_mkvec(yu_vec v, yu_num *out) {
    out->what = YU_NUM_VEC;
    out->n.v = v;
}

void yu_mkmat(yu_mat m, yu_num *out) {
    out->what = YU_NUM_MAT;
    out->n.m = m;
}

YU_ERR_RET yu_num_add(yu_num *a, yu_num *b, yu_num *out) {
    YU_ERR_DEFVAR
    yu_num *tmp;

#define SWAP(a,b) do{ tmp = a; a = b; b = tmp; }while(0)

    // Cleanest way I could think of to do it, but
    // augh THIS IS AWFUL.
    // TODO: NOT THIS
    switch (a->what) {
    case YU_NUM_FIXNUM: goto fxa;
    case YU_NUM_REAL: goto rla;
    case YU_NUM_RAT: goto rta;
    case YU_NUM_VEC: goto vca;
    case YU_NUM_MAT: goto mta;
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    fxa: switch (b->what) {
    case YU_NUM_FIXNUM: goto fxafxb;
    case YU_NUM_REAL: goto fxarlb;
    case YU_NUM_RAT: goto fxartb;
    case YU_NUM_VEC: goto fxavcb;
    case YU_NUM_MAT: goto fxamtb;
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    rla: switch (b->what) {
    case YU_NUM_FIXNUM: SWAP(a,b); goto fxarlb;
    case YU_NUM_REAL: goto rlarlb;
    case YU_NUM_RAT: goto rlartb;
    case YU_NUM_VEC: goto rlavcb;
    case YU_NUM_MAT: goto rlamtb;
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    rta: switch (b->what) {
    case YU_NUM_FIXNUM: SWAP(a,b); goto fxartb;
    case YU_NUM_REAL: SWAP(a,b); goto rlartb;
    case YU_NUM_RAT: goto rtartb;
    case YU_NUM_VEC: goto rtavcb;
    case YU_NUM_MAT: goto rtamtb;
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    vca: switch (b->what) {
    case YU_NUM_FIXNUM: SWAP(a,b); goto fxavcb;
    case YU_NUM_REAL: SWAP(a,b); goto rlavcb;
    case YU_NUM_RAT: SWAP(a,b); goto rtavcb;
    case YU_NUM_VEC: goto vcavcb;
    case YU_NUM_MAT: goto vcamtb;
    default: YU_THROW(YU_ERR_UNKNOWN);
    }

    mta: switch (b->what) {
    case YU_NUM_FIXNUM: SWAP(a,b); goto fxamtb;
    case YU_NUM_REAL: SWAP(a,b); goto rlamtb;
    case YU_NUM_RAT: SWAP(a,b); goto rtamtb;
    case YU_NUM_VEC: SWAP(a,b); goto vcamtb;
    case YU_NUM_MAT: goto mtamtb;
    }

    fxafxb: {
        yu_mkint(a->n.i + b->n.i, out);
        return 0;
    }
    fxarlb: {
        mpfr_t res;
        mpfr_init(res);
        mpfr_add_si(res, b->n.r, a->n.i, MPFR_RNDN);
        yu_mkreal(res, out);
        return 0;
    }
    fxartb: {
        mpq_t res, lhs;
        mpq_init(res);
        mpq_init(lhs);
        mpq_set_si(lhs, a->n.i, 1);
        mpq_add(res, lhs, b->n.q);
        yu_mkrat(res, out);
        return 0;
    }
    fxavcb: {
        if (a->n.i >= -32768 && a->n.i < 32767) {
            for (s64 i = 0; i < yu_vec_len(b->n.v); i++)
                b->n.v[i] += a->n.i;
            return 0;
        }
        else
            return YU_ERR_SCALAR_OPERAND_TOO_BIG;
    }
    fxamtb: {
        if (a->n.i >= -32768 && a->n.i < 32767) {
            s64 l = yu_mat_cols(b->n.m) * yu_mat_rows(b->n.m);
            for (s64 i = 0; i < l; i++)
                b->n.m[i] += a->n.i;
            *out = *b;
            return 0;
        }
        else
            return YU_ERR_SCALAR_OPERAND_TOO_BIG;
    }

    rlarlb: {
        mpfr_t res;
        mpfr_init(res);
        mpfr_add(res, a->n.r, b->n.r, MPFR_RNDN);
        yu_mkreal(res, out);
        return 0;
    }
    rlartb: {
        mpfr_t res;
        mpfr_init(res);
        mpfr_add_q(res, a->n.r, b->n.q, MPFR_RNDN);
        yu_mkreal(res, out);
        return 0;
    }
    rlavcb: {
        if (mpfr_cmp_si(a->n.r, -32768) >= 0 && mpfr_cmp_si(a->n.r, 32767) <= 0) {
            float r = mpfr_get_flt(a->n.r, MPFR_RNDN);
            for (s64 i = 0; i < yu_vec_len(b->n.v); i++)
                b->n.v[i] += r;
            return 0;
        }
        else
            return YU_ERR_SCALAR_OPERAND_TOO_BIG;
    }
    rlamtb: {
        if (mpfr_cmp_si(a->n.r, -32768) >= 0 && mpfr_cmp_si(a->n.r, 32767) <= 0) {
            float r = mpfr_get_flt(a->n.r, MPFR_RNDN);
            s64 l = yu_mat_cols(b->n.m) * yu_mat_rows(b->n.m);
            for (s64 i = 0; i < l; i++)
                b->n.m[i] += r;
            return 0;
        }
        else
            return YU_ERR_SCALAR_OPERAND_TOO_BIG;
    }

    rtartb: {
        mpq_t res;
        mpq_init(res);
        mpq_add(res, a->n.q, b->n.q);
        yu_mkrat(res, out);
        return 0;
    }
    rtavcb: {
        *tmp = (yu_num){.what = YU_NUM_REAL};
        mpfr_init_set_q(tmp->n.r, a->n.q, MPFR_RNDN);
        a = tmp;
        goto rlavcb;
    }
    rtamtb: {
        *tmp = (yu_num){.what = YU_NUM_REAL};
        mpfr_init_set_q(tmp->n.r, a->n.q, MPFR_RNDN);
        a = tmp;
        goto rlamtb;
    }

    vcavcb: {
        return 0;
    }
    vcamtb: {
        return 0;
    }

    mtamtb: {
        return 0;
    }

    YU_ERR_DEFAULT_HANDLER(yu_local_err)

#undef SWAP
}
