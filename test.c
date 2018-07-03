#include <math.h>

#define ANQOU_ASSERT assert

FILE *fmemopen(void *buf, size_t size, const char *mode);

void test_new_simple_token()
{
    Token *token;

    token = new_simple_token(TOKEN_LPAREN);
    ANQOU_ASSERT(token != NULL);
    ANQOU_ASSERT(token->kind == TOKEN_LPAREN);
    ANQOU_ASSERT(token->next == NULL);
    free(token);
}

void test_tokenize(const char *program, ...)
{
    va_list answers;
    FILE *fh;
    Token *token;

    fh = fmemopen((void *)program, strlen(program), "rb");
    token = tokenize(fh);
    ANQOU_ASSERT(token != NULL);
    fclose(fh);

    va_start(answers, program);
    while (token != NULL) {
        int ans = va_arg(answers, int);

        ANQOU_ASSERT(token->kind == ans);
        token = token->next;

        ANQOU_ASSERT(token != NULL || ans == TOKEN_EOF);
        ANQOU_ASSERT(ans != TOKEN_EOF || token == NULL);
    }
    va_end(answers);
}

void execute_test()
{
    test_new_simple_token();

    test_tokenize("0+0;", TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC,
                  TOKEN_SEMICOLON, TOKEN_EOF);
    test_tokenize("0+0+0+0+0;", TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC,
                  TOKEN_ADD, TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC, TOKEN_ADD,
                  TOKEN_NUMERIC, TOKEN_SEMICOLON, TOKEN_EOF);
    test_tokenize("0+0*0+0+0;", TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC,
                  TOKEN_MUL, TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC, TOKEN_ADD,
                  TOKEN_NUMERIC, TOKEN_SEMICOLON, TOKEN_EOF);
    test_tokenize("0+0*0+0-0;", TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC,
                  TOKEN_MUL, TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC, TOKEN_SUB,
                  TOKEN_NUMERIC, TOKEN_SEMICOLON, TOKEN_EOF);
    test_tokenize("(0+0)*0+0-0;", TOKEN_LPAREN, TOKEN_NUMERIC, TOKEN_ADD,
                  TOKEN_NUMERIC, TOKEN_RPAREN, TOKEN_MUL, TOKEN_NUMERIC,
                  TOKEN_ADD, TOKEN_NUMERIC, TOKEN_SUB, TOKEN_NUMERIC,
                  TOKEN_SEMICOLON, TOKEN_EOF);
    test_tokenize("asdg", TOKEN_LPAREN, TOKEN_NUMERIC, TOKEN_ADD, TOKEN_NUMERIC,
                  TOKEN_RPAREN, TOKEN_MUL, TOKEN_NUMERIC, TOKEN_ADD,
                  TOKEN_NUMERIC, TOKEN_SUB, TOKEN_NUMERIC, TOKEN_SEMICOLON,
                  TOKEN_EOF);
}
