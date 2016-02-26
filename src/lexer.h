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
    X(TOK_DEF, "definition") \
    X(TOK_WQUOT, "single-word quotation") \
    X(TOK_QUOT_START, "quotation start") \
    X(TOK_QUOT_END, "quotation end") \
    X(TOK_TAGV_START, "tagged value start") \
    X(TOK_TAGV_END, "tagged value end")

DEF_ENUM(token_type, LIST_TOKEN_TYPES)

struct token {
    token_type what;
    unsigned char *s;
    u64 len;
};

struct lexer {
    void *scanner;
    void *buf;
    FILE *in;
};

YU_ERR_RET lexer_open(struct lexer *lex, FILE *in);
void lexer_close(struct lexer *lex);
bool lexer_next(struct lexer *lex, struct token *out);

void lexer_free_token(struct lexer *lex, struct token *t);


void print_token(struct token *t, FILE *out);
