#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0

typedef enum TOKEN_KIND TOKEN_KIND;
typedef struct Token Token;

enum {
    TOKEN_NUMERIC,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_VARIABLE,
    TOKEN_OPERATOR,
};

struct Token {
    union {
        float num;
        char str[10];
    } data;
    int kind;

    Token *next;
};

Token *new_numeric_token(float num)
{
    Token *token;

    token = (Token *)malloc(sizeof(Token));
    token->next = NULL;
    token->data.num = num;
    token->kind = TOKEN_NUMERIC;
    return token;
}

enum {
    TK_ST_INITIAL,
    TK_ST_NUMERIC,
    TK_ST_VARIABLE,
};

Token *tokenize(FILE *fp, int eval)
{
    char buf[100];
    int bufidx, ch, st = TK_ST_INITIAL;
    Token *token_list_tail = NULL, *token_list_head = NULL;

    while ((ch = fgetc(fp)) != EOF) {
        Token *token = NULL;

        switch (st) {
            case TK_ST_INITIAL:
                if (eval && ch == '\n') goto ret;
                if (isspace(ch)) break;
                if (isdigit(ch)) {
                    bufidx = 0;
                    assert(bufidx < sizeof(buf) - 1);
                    buf[bufidx++] = ch;
                    st = TK_ST_NUMERIC;
                    break;
                }

                return NULL;

            case TK_ST_NUMERIC:
                if (isdigit(ch) || ch == '.') {
                    assert(bufidx < sizeof(buf) - 1);
                    buf[bufidx++] = ch;
                    break;
                }

                assert(bufidx < sizeof(buf) - 1);
                buf[bufidx++] = '\0';
                token = new_numeric_token(atof(buf));

                st = TK_ST_INITIAL;

                ungetc(ch, fp);
                break;

            default:
                assert(false);
        }

        if (token != NULL) {
            if (token_list_tail != NULL) token_list_tail->next = token;
            token_list_tail = token;
            if (token_list_head == NULL) token_list_head = token_list_tail;
        }
    }

ret:

    return token_list_head;
}

void free_token_list(Token *token_list)
{
    while (token_list != NULL) {
        Token *token = token_list;

        token_list = token_list->next;
        free(token);
    }
}

#include "test.c"

int main(void)
{
    Token *token, *token_list;

    execute_test();

    while (1) {
        token_list = tokenize(stdin, 1);
        if (token_list == NULL) {
            printf("Error: can't tokenize.\n");
            return 1;
        }

        for (token = token_list; token != NULL; token = token->next) {
            switch (token->kind) {
                case TOKEN_NUMERIC:
                    printf("%f ", token->data.num);
                    break;

                default:
                    assert(false);
            }
            puts("");
        }

        free_token_list(token_list);
    }

    return 0;
}
