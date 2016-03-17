%option reentrant stack noyywrap
%option noyyalloc noyyrealloc noyyfree

%{
#include "utf8proc/utf8proc.h"

#define YY_EXTRA_TYPE struct lexer *

#define EMIT_OFFSET(type, offs) do{ \
    if (type != TOK_ERR && type != TOK_STR && \
        yu_str_new(&yyextra->str_ctx, (unsigned char *)yytext+offs, yyleng-offs, &yyextra->active_s) != YU_OK) { \
        yyextra->err_msg = "Internal lexer failure—bad UTF8-encoded input?"; \
        return TOK_ERR; \
    } \
    return type; \
}while(0)

#define EMIT(type) EMIT_OFFSET(type, 0)

#define ERROR(msg) do{ \
    yyextra->err_msg = msg; \
    EMIT(TOK_ERR); \
}while(0)

#define S_START(init,len) do{ \
    if (yu_str_new(&yyextra->str_ctx, (unsigned char *)init, len, &yyextra->active_s) != YU_OK) { \
        yyextra->err_msg = "Internal lexer exception"; \
        return TOK_ERR; \
    } \
}while(0)

#define S_CAT(str,len) do{ \
    yu_str tmp; \
    if (yu_str_new(&yyextra->str_ctx, (unsigned char *)str, len, &tmp) != YU_OK) { \
        yyextra->err_msg = "Internal lexer exception"; \
        return TOK_ERR; \
    } \
    if (yu_str_cat(yyextra->active_s, tmp, &yyextra->active_s) != YU_OK) { \
        yyextra->err_msg = "Internal lexer exception"; \
        yu_str_free(tmp); \
        return TOK_ERR; \
    } \
    yu_str_free(tmp); \
}while(0)

%}

NAME	[^[:digit:][:space:]\(\)\[\]\{:.$"!\-][^[:space:]\[\]\{\}\(\):.$"]*\{?

%x STR RAW_STR

%%

\"\"\"					yy_push_state(RAW_STR, yyscanner);
<RAW_STR>[^(""")]*			S_START(yytext, yyleng); EMIT(TOK_STR);
<RAW_STR>\"\"\"				yy_pop_state(yyscanner);
\"					yy_push_state(STR, yyscanner); S_START("",0);
<STR>\n					ERROR("Unterimated string literal");
<STR>\\n				S_CAT("\n",1);
<STR>\\r				S_CAT("\r",1);
<STR>\\t				S_CAT("\t",1);
<STR>\\(.|\n)				S_CAT(yytext+1,1);
<STR>\\u\{(0x)?[0-9a-fA-F]+\}		{
    u8 offset = 3;
    if (yytext[3] == '0' && yytext[4] == 'x')
        offset = 5;
    u32 codepoint;
    u8 utf8_len, as_utf8[4];
    if (sscanf(yytext+offset, "%x", &codepoint) == EOF)
        ERROR("Invalid Unicode escape sequence");
    if ((utf8_len = utf8proc_encode_char(codepoint, as_utf8)) < 1)
        ERROR("Invalid Unicode escape sequence");
    S_CAT(as_utf8, utf8_len);
}
<STR>\\u\{				ERROR("Invalid Unicode escape sequence");
<STR>[^\\\n"]+				S_CAT(yytext,yyleng);
<STR>\"					yy_pop_state(yyscanner); EMIT(TOK_STR);
{NAME}\.{NAME}				EMIT(TOK_WORD);
{NAME}:					EMIT(TOK_DEF);
{NAME}					EMIT(TOK_WORD);
\.{NAME}				EMIT(TOK_LENS);
\.[[:digit:]]+				EMIT(TOK_LENS);
\.					EMIT(TOK_WORD);
!{NAME}					EMIT_OFFSET(TOK_PRAGMA, 1);
\${NAME}				EMIT_OFFSET(TOK_TAG, 1);
\'{NAME}				EMIT(TOK_WQUOT);
-?[[:digit:]]+\.[[:digit:]]+		EMIT(TOK_REAL);
-?[[:digit:]]+				EMIT(TOK_INT);
::					EMIT(TOK_STACK_DEF);
\[					EMIT(TOK_QUOT_START);
\]					EMIT(TOK_QUOT_END);
\(					EMIT(TOK_TAGV_START);
\)					EMIT(TOK_TAGV_END);

%%

void *yyalloc(size_t bytes, yyscan_t scanner) {
    return yu_xalloc(yyget_extra(scanner)->mem_ctx, bytes, 1);
}

void *yyrealloc(void *ptr, size_t bytes, yyscan_t scanner) {
    return yu_xrealloc(yyget_extra(scanner)->mem_ctx, ptr, bytes, 1);
}

void yyfree(void *ptr, yyscan_t scanner) {
    yu_free(yyget_extra(scanner)->mem_ctx, ptr);
}
