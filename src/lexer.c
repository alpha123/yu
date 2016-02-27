#include "lexer.h"

#include "lex.i"

YU_ERR_RET lexer_open(struct lexer *lex, FILE *in) {
    YU_ERR_DEFVAR
    lex->in = in;
    YU_THROWIF(yylex_init(&lex->scanner) != 0, YU_ERR_UNKNOWN_FATAL);
    lex->buf = yy_create_buffer(in, YY_BUF_SIZE, lex->scanner);
    YU_THROWIFNULL(lex->buf, YU_ERR_UNKNOWN_FATAL);
    yy_switch_to_buffer(lex->buf, lex->scanner);
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void lexer_close(struct lexer *lex) {
    fclose(lex->in);
    yy_delete_buffer(lex->buf, lex->scanner);
    yylex_destroy(lex->scanner);
}

bool lexer_next(struct lexer *lex, struct token *out) {
    if ((out->what = yylex(lex->scanner))) {
        out->len = yyget_leng(lex->scanner);
        out->s = calloc(out->len + 1, 1);
        if (out->s == NULL) {
            out->what = TOK_ERR;
            out->s = (unsigned char *)"failed to allocate memory";
            return false;
        }
        memcpy(out->s, yyget_text(lex->scanner), out->len);
        // s[len] == 0 because we allocated with calloc
        return true;
    }
    return false;
}

void lexer_free_token(struct lexer *lex, struct token *t) {
    if (t->what != TOK_ERR)
        free(t->s);
}


static
const char *get_type_desc(token_type type) {
#define CASE_TYPE(t, desc) case t: return desc;
    switch (type) {
    LIST_TOKEN_TYPES(CASE_TYPE)
    default: return "unknown";
    }
#undef CASE_TYPE
}

void print_token(struct token *t, FILE *out) {
    fprintf(out, "TOKEN (%" PRIu64 ") %.*s %s ;", t->len, (s32)t->len, t->s, get_type_desc(t->what));
}
