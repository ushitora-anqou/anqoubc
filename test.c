#define ANQOU_ASSERT assert
#include <math.h>

void test_tokenize(const char *filepath,
                   void (*testfunc)(Token *token_list, FILE *fp))
{
    FILE *fp = NULL;
    Token *token_list;

    fp = fopen(filepath, "r");
    ANQOU_ASSERT(fp != NULL);
    token_list = tokenize(fp, 0);
    fclose(fp);

    fp = fopen(filepath, "r");
    ANQOU_ASSERT(fp != NULL);
    ANQOU_ASSERT(token_list != NULL);
    testfunc(token_list, fp);
    fclose(fp);

    free_token_list(token_list);
}

void test_tokenize_numeric(Token *token_list, FILE *fp)
{
    float true_value;
    Token *token;

    for (token = token_list; token != NULL; token = token->next) {
        fscanf(fp, "%f", &true_value);
        ANQOU_ASSERT(token->kind == TOKEN_NUMERIC);
        ANQOU_ASSERT(fabs(true_value - token->data.num) < 10e-5);
    }
}

void test_tokenize_add(Token *token_list, FILE *fp)
{
    Token *token;

    for (token = token_list; token != NULL; token = token->next) {
        float lhs_true_value, rhs_true_value;

        fscanf(fp, "%f+%f", &lhs_true_value, &rhs_true_value);

        ANQOU_ASSERT(token->kind == TOKEN_NUMERIC);
        ANQOU_ASSERT(fabs(lhs_true_value - token->data.num) < 10e-5);

        token = token->next;
        ANQOU_ASSERT(token != NULL);
        ANQOU_ASSERT(token->kind == TOKEN_ADD);

        token = token->next;
        ANQOU_ASSERT(token != NULL);
        ANQOU_ASSERT(token->kind == TOKEN_NUMERIC);
        ANQOU_ASSERT(fabs(rhs_true_value - token->data.num) < 10e-5);
    }
}

void execute_test()
{
    test_tokenize("test/test_tokenize_numeric.dat", test_tokenize_numeric);
    test_tokenize("test/test_tokenize_add.dat", test_tokenize_add);
    return;
}
