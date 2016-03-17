/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "lexer.h"

#include "lex.i"

YU_ERR_RET lexer_open(struct lexer *lex, FILE *in, yu_memctx_t *mctx) {
    YU_ERR_DEFVAR
    lex->mem_ctx = mctx;
    yu_str_ctx_init(&lex->str_ctx, mctx);
    lex->in = in;
    YU_THROWIF(yylex_init_extra(lex, &lex->scanner) != 0, YU_ERR_UNKNOWN_FATAL);
    lex->buf = yy_create_buffer(in, YY_BUF_SIZE, lex->scanner);
    YU_THROWIFNULL(lex->buf, YU_ERR_UNKNOWN_FATAL);
    yy_switch_to_buffer(lex->buf, lex->scanner);
    YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void lexer_close(struct lexer *lex) {
    yu_str_ctx_free(&lex->str_ctx);
    fclose(lex->in);
    yy_delete_buffer(lex->buf, lex->scanner);
    yylex_destroy(lex->scanner);
}

bool lexer_next(struct lexer *lex, struct token *out) {
    if ((out->what = yylex(lex->scanner))) {
        out->s = lex->active_s;
        return true;
    }
    return false;
}

void lexer_free_token(struct lexer * YU_UNUSED(lex), struct token * YU_UNUSED(t)) {
}

const char *lexer_get_token_type_name(token_type type) {
#define CASE_TYPE(t, desc) case t: return desc;
    switch (type) {
    LIST_TOKEN_TYPES(CASE_TYPE)
    default: return "unknown";
    }
#undef CASE_TYPE
}
