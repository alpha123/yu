/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"

#include "lexer.h"

#define lex_init_str(s) do{ \
    FILE *str = fmemopen(s, strlen(s), "r"); \
    yu_err ser = lexer_open(&lex, str, &mctx); \
    assert(ser == YU_OK); \
}while(0)

#define ASSERT_TOK(type, str) do{ \
    PT_ASSERT(lexer_next(&lex, &tok)); \
    PT_ASSERT_EQ(tok.what, type); \
    yu_str check; \
    yu_err ser = yu_str_new_z(&lex.str_ctx, str, &check); \
    assert(ser == YU_OK); \
    ASSERT_YU_STR_EQ(tok.s, check); \
    yu_str_free(check); \
    lexer_free_token(&lex, &tok); \
}while(0)

#define ASSERT_LAST_TOK(type, str) do{ \
    ASSERT_TOK(type, str); \
    PT_ASSERT(!lexer_next(&lex, &tok)); \
}while(0)

#define SETUP \
    yu_memctx_t mctx; \
    TEST_GET_ALLOCATOR(&mctx); \
    struct lexer lex; \
    struct token tok;

#define TEARDOWN \
    lexer_close(&lex); \
    yu_alloc_ctx_free(&mctx);

#define LIST_LEXER_TESTS(X) \
    X(int, "Positive and negative integers should be recognized") \
    X(real, "Positive and negative real numbers should be recognized") \
    X(tag, "Tags should begin with $ and be a word") \
    X(str, "Strings should be enclosed in quotes and must not contain newlines") \
    X(str_escape, "Strings should expand escape characters") \
    X(str_unicode, "Strings should expand unicode codepoints, including those beyond the BMP") \
    X(rawstr, "Raw strings may contain newlines and don't expand escape characters") \
    X(symbols, "Lexer should recognize a few special symbols") \
    X(compound, "Lexer should recognize many tokens in sequence")

TEST(int)
    lex_init_str("42");
    ASSERT_LAST_TOK(TOK_INT, "42");
    lexer_close(&lex);
    lex_init_str("-322");
    ASSERT_LAST_TOK(TOK_INT, "-322");
END(int)

TEST(real)
    lex_init_str("3.14159265358979");
    ASSERT_LAST_TOK(TOK_REAL, "3.14159265358979");
    lexer_close(&lex);
    lex_init_str("-2.718281828");
    ASSERT_LAST_TOK(TOK_REAL, "-2.718281828");
END(real)

TEST(tag)
    lex_init_str("$foo");
    ASSERT_LAST_TOK(TOK_TAG, "foo");
END(tag)

TEST(str)
    lex_init_str("\"what a cute little string~!\"");
    ASSERT_LAST_TOK(TOK_STR, "what a cute little string~!");
    lexer_close(&lex);
    lex_init_str("\"illegal\nstring\"");
    lexer_next(&lex, &tok);
    PT_ASSERT_EQ(tok.what, TOK_ERR);
    lexer_free_token(&lex, &tok);
END(str)

TEST(str_escape)
    lex_init_str("\"string with\\nnewline\"");
    ASSERT_LAST_TOK(TOK_STR, "string with\nnewline");
    lexer_close(&lex);
    lex_init_str("\"anot\\\"her\\tstring\"");
    ASSERT_LAST_TOK(TOK_STR, "anot\"her\tstring");
END(str_escape)

TEST(str_unicode)
    lex_init_str("\"\\u{0x0055}nicode \\u{01d4ae}tring\"");
    ASSERT_LAST_TOK(TOK_STR, "Unicode 𝒮tring");
    lexer_close(&lex);
    lex_init_str("\"invalid\\u{string}\"");
    lexer_next(&lex, &tok);
    PT_ASSERT_EQ(tok.what, TOK_ERR);
    lexer_free_token(&lex, &tok);
END(str_unicode)

TEST(rawstr)
    lex_init_str("\"\"\"raw strings\ncan\\u{have}\\n whatever\"\"\"");
    ASSERT_LAST_TOK(TOK_STR, "raw strings\ncan\\u{have}\\n whatever");
END(rawstr)

TEST(symbols)
    lex_init_str("[](){}");
    ASSERT_TOK(TOK_QUOT_START, "[");
    ASSERT_TOK(TOK_QUOT_END, "]");
    ASSERT_TOK(TOK_TUPLE_START, "(");
    ASSERT_TOK(TOK_TUPLE_END, ")");
    ASSERT_TOK(TOK_TABLE_START, "{");
    ASSERT_LAST_TOK(TOK_TABLE_END, "}");
END(symbols)

TEST(compound)
    lex_init_str("do-thing::𝑓 -> 𝑥;do-thing:2 4.0 + 'dip keep [2 * bi>>] {\"x\" \"y\"} $point(x y) from-hash;");
    ASSERT_TOK(TOK_STACK_DEF, "do-thing");
    ASSERT_TOK(TOK_WORD, "𝑓");
    ASSERT_TOK(TOK_WORD, "->");
    ASSERT_TOK(TOK_WORD, "𝑥");
    ASSERT_TOK(TOK_WORD, ";");
    ASSERT_TOK(TOK_DEF, "do-thing");
    ASSERT_TOK(TOK_INT, "2");
    ASSERT_TOK(TOK_REAL, "4.0");
    ASSERT_TOK(TOK_WORD, "+");
    ASSERT_TOK(TOK_WQUOT, "dip");
    ASSERT_TOK(TOK_WORD, "keep");
    ASSERT_TOK(TOK_QUOT_START, "[");
    ASSERT_TOK(TOK_INT, "2");
    ASSERT_TOK(TOK_WORD, "*");
    ASSERT_TOK(TOK_WORD, "bi>>");
    ASSERT_TOK(TOK_QUOT_END, "]");
    ASSERT_TOK(TOK_TABLE_START, "{");
    ASSERT_TOK(TOK_STR, "x");
    ASSERT_TOK(TOK_STR, "y");
    ASSERT_TOK(TOK_TABLE_END, "}");
    ASSERT_TOK(TOK_TAG, "point");
    ASSERT_TOK(TOK_TUPLE_START, "(");
    ASSERT_TOK(TOK_WORD, "x");
    ASSERT_TOK(TOK_WORD, "y");
    ASSERT_TOK(TOK_TUPLE_END, ")");
    ASSERT_TOK(TOK_WORD, "from-hash");
    ASSERT_TOK(TOK_WORD, ";");
END(compound)


SUITE(lexer, LIST_LEXER_TESTS)
