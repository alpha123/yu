/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include "yu_common.h"

#define LIST_TOKEN_TYPES(X) \
    X(TOK_EOF, "eof") \
    X(TOK_ERR, "error") \
    X(TOK_INT, "integer") \
    X(TOK_REAL, "real") \
    X(TOK_TAG, "tag") \
    X(TOK_STR, "string") \
    X(TOK_WORD, "word") \
    X(TOK_LENS, "lens") \
    X(TOK_PRAGMA, "pragma") \
    X(TOK_DEF, "definition") \
    X(TOK_STACK_DEF, "stack effect definition") \
    X(TOK_WQUOT, "single-word quotation") \
    X(TOK_QUOT_START, "quotation start") \
    X(TOK_QUOT_END, "quotation end") \
    X(TOK_TUPLE_START, "tuple literal start") \
    X(TOK_TUPLE_END, "tuple literal end") \
    X(TOK_TABLE_START, "table literal start") \
    X(TOK_TABLE_END, "table literal end")

DEF_ENUM(token_type, LIST_TOKEN_TYPES)

struct token {
    yu_str s;
    token_type what;
};

struct lexer {
    yu_str_ctx str_ctx;
    yu_memctx_t *mem_ctx;

    void *scanner;
    void *buf;
    FILE *in;

    yu_str active_s;

    char *err_msg;
};

YU_ERR_RET lexer_open(struct lexer *lex, FILE *in, yu_memctx_t *mctx);
void lexer_close(struct lexer *lex);
bool lexer_next(struct lexer *lex, struct token *out);

void lexer_free_token(struct lexer *lex, struct token *t);


const char *lexer_get_token_type_name(token_type type);
