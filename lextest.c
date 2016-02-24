#include "yu_common.h"
#include "lexer.h"
#include "linenoise.h"

int main(int argc, const char **argv) {
    char *line;
    struct lexer lxr;
    struct token tok;
    while ((line = linenoise("tok> "))) {
        if (line[0] != '\0')
            linenoiseHistoryAdd(line);
        if (lexer_open(&lxr, fmemopen(line, strlen(line), "r"))) {
            printf("unknown error\n");
            exit(1);
        }
        while (lexer_next(&lxr, &tok)) {
            if (tok.what == TOK_ERR)
                printf("error: %*.s\n", (s32)tok.len, tok.s);
            else {
                if (strcmp((char *)tok.s, "exit") == 0)
                    exit(0);
                print_token(&tok, stdout);
                putchar(' ');
            }
            lexer_free_token(&lxr, &tok);
        }
        lexer_close(&lxr);
        free(line);
        putchar('\n');
    }
    return 0;
}
