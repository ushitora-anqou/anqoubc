#include <math.h>

#define ANQOU_ASSERT assert

FILE *fmemopen(void *buf, size_t size, const char *mode);

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

        ANQOU_ASSERT(token != NULL || ans == tEOF);
        ANQOU_ASSERT(ans != tEOF || token == NULL);
    }
    va_end(answers);
}

void execute_test()
{
    test_tokenize("0+0;", tFLOAT, tADD, tFLOAT, tSEMICOLON, tEOF);
    test_tokenize("0+0+0+0+0;", tFLOAT, tADD, tFLOAT, tADD, tFLOAT, tADD,
                  tFLOAT, tADD, tFLOAT, tSEMICOLON, tEOF);
    test_tokenize("0+0*0+0+0;", tFLOAT, tADD, tFLOAT, tMUL, tFLOAT, tADD,
                  tFLOAT, tADD, tFLOAT, tSEMICOLON, tEOF);
    test_tokenize("0+0*0+0-0;", tFLOAT, tADD, tFLOAT, tMUL, tFLOAT, tADD,
                  tFLOAT, tSUB, tFLOAT, tSEMICOLON, tEOF);
    test_tokenize("(0+0)*0+0-0;", tLPAREN, tFLOAT, tADD, tFLOAT, tRPAREN, tMUL,
                  tFLOAT, tADD, tFLOAT, tSUB, tFLOAT, tSEMICOLON, tEOF);
}
