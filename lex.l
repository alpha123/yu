%option reentrant stack noyywrap

NAME	[^[:digit:][:space:]\(\)\[\]\{:.$&][^[:space:]\[\]\{\}\(\):.$"]*\{?

%x STR

%%

{NAME}::{NAME}				return TOK_WORD;
{NAME}:					return TOK_DEF;
{NAME}					return TOK_WORD;
\.{NAME}				return TOK_LENS;
\.[[:digit:]]+				return TOK_LENS;
\.					return TOK_WORD;
\${NAME}				return TOK_TAG;
&{NAME}					return TOK_WQUOT;
[[:digit:]]+\.[[:digit:]]+		return TOK_REAL;
[[:digit:]]+				return TOK_INT;
\[					return TOK_QUOT_START;
\]					return TOK_QUOT_END;
\(					return TOK_TAGV_START;
\)					return TOK_TAGV_END;

%%
