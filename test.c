#define ANQOU_ASSERT assert
#include <math.h>

void test_tokenize_numeric()
{
    FILE *fp = NULL;
    Token *token, *token_list;

    fp = fopen("test/test_numeric.dat", "r");
    ANQOU_ASSERT(fp != NULL);
    token_list = tokenize(fp, 0);
    fclose(fp);

    fp = fopen("test/test_numeric.dat", "r");
    ANQOU_ASSERT(fp != NULL);
    ANQOU_ASSERT(token_list != NULL);
    for (token = token_list; token != NULL; token = token->next) {
        float true_value;

        fscanf(fp, "%f", &true_value);
        ANQOU_ASSERT(token->kind == TOKEN_NUMERIC);
        ANQOU_ASSERT(fabs(true_value - token->data.num) < 10e-5);
    }
    fclose(fp);

    free_token_list(token_list);
}

void execute_test()
{
    test_tokenize_numeric();
    return;
}
