#include <stdio.h>
#include "yu_common.h"

void print_vec(yu_vec v) {
    putchar('{');
    putchar(' ');
    for (s64 i = 0; i < yu_vec_len(v); i++)
        printf("%f ", v[i]);
    putchar('}');
    putchar('\n');
}

int main(int argc, char **argv) {
    YU_ERR_DEFVAR
    yu_num a, b, c, v, w;
    yu_vec vv, ww;
    mpfr_t r;
    mpq_t p, q;
    mpfr_init(r);
    mpq_init(p);
    mpq_init(q);

    yu_mkint(15, &a);
    yu_mkint(77, &b);
    YU_CHECK(yu_num_add(&a, &b, &c));
    printf("%d + %d = %d\n", a.n.i, b.n.i, c.n.i);

    mpfr_set_d(r, 3.22, MPFR_RNDN);
    yu_mkreal(r, &a);
    YU_CHECK(yu_num_add(&a, &b, &c));
    mpfr_printf("%.2Rf + %d = %.2Rf\n", a.n.r, b.n.i, c.n.r);
    mpfr_clear(c.n.r);

    mpq_set_si(p, 1, 3);
    mpfr_set_d(r, 2.667, MPFR_RNDN);
    yu_mkrat(p, &a);
    yu_mkreal(r, &b);
    YU_CHECK(yu_num_add(&a, &b, &c));
    mpfr_printf("%.Qd + %.3Rf = %.3Rf\n", a.n.q, b.n.r, c.n.r);
    mpfr_clear(c.n.r);

    vv = yu_vec_new(10);
    ww = yu_vec_new(10);
    for (int i = 0; i < 10; i++) {
        vv[i] = i + 1;
        ww[i] = 10 - i;
    }
    print_vec(vv);
    print_vec(ww);

    yu_mkvec(vv, &v);
    yu_mkvec(ww, &w);

    YU_CHECK(yu_num_add(&v, &a, &v));

    print_vec(vv);

    yu_vec_free(vv);
    yu_vec_free(ww);

    return 0;

    yu_err_handler:
    printf("ERROR: %s\n", yu_err_msg(yu_local_err));
    return yu_local_err;
}
