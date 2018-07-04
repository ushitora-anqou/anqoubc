#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0

float fmax(float lhs, float rhs) { return lhs < rhs ? rhs : lhs; }

enum {
    tNUMBER,
    tLPAREN,
    tRPAREN,
    tVARIABLE,
    tADD,
    tSUB,
    tMUL,
    tDIV,
    tSEMICOLON,
    tEOF,
};

typedef struct Token {
    int kind;

    union {
        float num;
        char str[10];
    } data;

    struct Token *next;
} Token;

enum {
    TY_INT,
    TY_FLOAT,
};

typedef struct {
    int kind;
} Type;

enum { AST_LITERAL, AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_PROG };

typedef struct AST AST;
struct AST {
    int kind;
    Type type;

    union {
        /* AST_PROG */
        struct {
            AST *stmt;
            struct AST *next;
        };

        /* AST_LITERAL */
        float fval;
        int ival;

        /* AST_ADD, AST_SUB, AST_MUL, AST_DIV */
        struct {
            AST *lhs, *rhs;
        };
    };
};

Token *new_simple_token(int kind);
Token *new_number_token(float num);
Token *tokenize(FILE *fp);
void free_token_list(Token *token_list);
AST *new_ast_float(float num);
AST *new_ast_binary_op(int kind, AST *lhs, AST *rhs);
Token *pop_token(Token **token_list);
Token *peek_token(Token **token_list);
Token *pop_number_token(Token **token_list);
Token *parse_match(Token **token_list, int kind);
AST *parse_factor(Token **token_list);
AST *parse_term_detail(Token **token_list, AST *factor);
AST *parse_term(Token **token_list);
AST *parse_expr_detail(Token **token_list, AST *term);
AST *parse_expr(Token **token_list);
AST *parse_prog(Token **token_list);
AST *parse(Token *token_list);
void dump_token_list(Token *token_list);

Token *new_simple_token(int kind)
{
    Token *token;

    token = (Token *)malloc(sizeof(Token));
    token->next = NULL;
    token->kind = kind;
    return token;
}

Token *new_number_token(float num)
{
    Token *token = new_simple_token(tNUMBER);

    token->data.num = num;
    return token;
}

enum {
    TK_ST_INITIAL,
    TK_ST_NUMBER,
    TK_ST_VARIABLE,
};

Token *tokenize(FILE *fp)
{
    char buf[256]; /* TODO: should be enough??? */
    int bufidx, st = TK_ST_INITIAL;
    Token *token = NULL, *token_list_tail = NULL, *token_list_head = NULL;

    while (true) {
        int ch;

        ch = fgetc(fp);
        if (ch == EOF) break;

        switch (st) {
            case TK_ST_INITIAL:
                if (isspace(ch)) break;
                if (isdigit(ch)) {
                    bufidx = 0;
                    assert(bufidx < sizeof(buf) - 1);
                    buf[bufidx++] = ch;
                    st = TK_ST_NUMBER;
                    break;
                }
                switch (ch) {
                    case '+':
                        token = new_simple_token(tADD);
                        break;
                    case '-':
                        token = new_simple_token(tSUB);
                        break;
                    case '*':
                        token = new_simple_token(tMUL);
                        break;
                    case '/':
                        token = new_simple_token(tDIV);
                        break;
                    case '(':
                        token = new_simple_token(tLPAREN);
                        break;
                    case ')':
                        token = new_simple_token(tRPAREN);
                        break;
                    case ';':
                        token = new_simple_token(tSEMICOLON);
                        break;
                }
                if (token != NULL) break;

                return NULL;

            case TK_ST_NUMBER:
                if (isdigit(ch) || ch == '.') {
                    assert(bufidx < sizeof(buf) - 1);
                    buf[bufidx++] = ch;
                    break;
                }

                assert(bufidx < sizeof(buf) - 1);
                buf[bufidx++] = '\0';
                token = new_number_token(atof(buf));

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

    token = new_simple_token(tEOF);
    /* TODO: duplicate code */
    if (token_list_tail != NULL) token_list_tail->next = token;
    token_list_tail = token;
    if (token_list_head == NULL) token_list_head = token_list_tail;

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

AST *new_ast_float(float val)
{
    AST *ast;

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = AST_LITERAL;
    ast->type.kind = TY_FLOAT;
    ast->fval = val;

    return ast;
}

AST *new_ast_binary_op(int kind, AST *lhs, AST *rhs)
{
    AST *ast;

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = kind;
    ast->type.kind = lhs->type.kind; /* TODO */
    ast->lhs = lhs;
    ast->rhs = rhs;

    return ast;
}

Token *pop_token(Token **token_list)
{
    Token *token = *token_list;

    if (token == NULL) return NULL;
    *token_list = (*token_list)->next;
    return token;
}

Token *peek_token(Token **token_list) { return *token_list; }

Token *pop_number_token(Token **token_list)
{
    Token *token = peek_token(token_list);

    if (token == NULL || token->kind != tNUMBER) return NULL;
    return pop_token(token_list);
}

Token *parse_match(Token **token_list, int kind)
{
    Token *token = peek_token(token_list);

    if (token == NULL) return NULL;
    if (token->kind == kind) return pop_token(token_list);
    return NULL;
}

AST *parse_factor(Token **token_list)
{
    AST *ast;

    if (parse_match(token_list, tLPAREN) != NULL) {
        /* (expr) */
        ast = parse_expr(token_list);
        assert(parse_match(token_list, tRPAREN) != NULL);
    }
    else {
        /* number */
        Token *token;

        token = pop_number_token(token_list);
        if (token == NULL) return NULL;

        dump_token_list(*token_list);

        ast = (AST *)new_ast_float(token->data.num);
    }

    return ast;
}

AST *parse_term_detail(Token **token_list, AST *factor)
{
    Token *op;
    AST *ast = factor;

    if ((op = parse_match(token_list, tMUL)) != NULL) {
        AST *lhs, *rhs;

        lhs = factor;
        rhs = parse_factor(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_MUL, lhs, rhs);
        ast = parse_term_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, tDIV)) != NULL) {
        AST *lhs, *rhs;

        lhs = factor;
        rhs = parse_factor(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_DIV, lhs, rhs);
        ast = parse_term_detail(token_list, ast);
    }

    return ast;
}

AST *parse_term(Token **token_list)
{
    AST *ast;
    Token *org_token_list_head = *token_list;

    ast = parse_factor(token_list);
    if (ast == NULL) goto err;

    ast = parse_term_detail(token_list, ast);
    if (ast == NULL) goto err;

    return ast;

err:
    *token_list = org_token_list_head;
    return NULL;
}

AST *parse_expr_detail(Token **token_list, AST *term)
{
    Token *op;
    AST *ast = term;

    if ((op = parse_match(token_list, tADD)) != NULL) {
        AST *lhs, *rhs;

        dump_token_list(*token_list);

        lhs = term;
        rhs = parse_term(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_ADD, lhs, rhs);
        ast = parse_expr_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, tSUB)) != NULL) {
        AST *lhs, *rhs;

        lhs = term;
        rhs = parse_term(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_SUB, lhs, rhs);
        ast = parse_expr_detail(token_list, ast);
    }

    return ast;
}

AST *parse_expr(Token **token_list)
{
    AST *ast;
    Token *org_token_list_head = *token_list;

    dump_token_list(*token_list);

    ast = parse_term(token_list);
    if (ast == NULL) goto err;

    dump_token_list(*token_list);

    ast = parse_expr_detail(token_list, ast);
    if (ast == NULL) goto err;

    return ast;

err:
    *token_list = org_token_list_head;
    return NULL;
}

AST *parse_stmt(Token **token_list)
{
    AST *ast;

    ast = parse_expr(token_list);
    if (ast == NULL) return NULL;
    assert(parse_match(token_list, tSEMICOLON) != NULL);
    dump_token_list(*token_list);

    return ast;
}

AST *parse_prog(Token **token_list)
{
    AST *stmt;
    AST *prog, *ast;

    if (parse_match(token_list, tEOF) != NULL) return NULL;

    stmt = parse_stmt(token_list);
    if (stmt == NULL) return NULL;
    prog = parse_prog(token_list);

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = AST_PROG;
    ast->stmt = stmt;
    ast->next = prog;

    return ast;
}

AST *parse(Token *token_list)
{
    AST *prog;

    prog = parse_prog(&token_list);
    assert(token_list == NULL);

    return prog;
}

/*
float eval_ast(AST *ast)
{
    switch (ast->kind) {
        case AST_PROG: {
            while (ast != NULL) {
                assert(ast->kind == AST_PROG);
                printf("%f\n", eval_ast(ast->stmt));
                ast = ast->next;
            }
            return 0;
        }

        case AST_ADD: {
            return eval_ast(ast->lhs) + eval_ast(ast->rhs);
        }

        case AST_SUB: {
            return eval_ast(ast->lhs) - eval_ast(ast->rhs);
        }

        case AST_MUL: {
            return eval_ast(ast->lhs) * eval_ast(ast->rhs);
        }

        case AST_DIV: {
            return eval_ast(ast->lhs) / eval_ast(ast->rhs);
        }
    }

    assert(false);
}
*/

void free_ast(AST *ast)
{
    if (ast == NULL) return;

    switch (ast->kind) {
        case AST_PROG: {
            free_ast(ast->stmt);
            free_ast((AST *)ast->next);
            free(ast);
            return;
        }

        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV: {
            free_ast(ast->lhs);
            free_ast(ast->rhs);
            free(ast);
            return;
        }

        case AST_LITERAL: {
            free(ast);
            return;
        }
    }

    assert(false);
}

void dump_token_list(Token *token)
{
    for (; token != NULL; token = token->next) {
        switch (token->kind) {
            case tNUMBER:
                printf("%f ", token->data.num);
                break;

            case tADD:
                printf("+ ");
                break;

            case tSUB:
                printf("- ");
                break;

            case tMUL:
                printf("* ");
                break;

            case tDIV:
                printf("/ ");
                break;

            case tLPAREN:
                printf("( ");
                break;

            case tRPAREN:
                printf(") ");
                break;

            case tSEMICOLON:
                printf("; ");
                break;

            case tEOF:
                printf("<EOF> ");
                break;

            default:
                printf("???%d\n", token->kind);
                assert(false);
        }
    }

    puts("");
}

typedef struct {
    char **data;
    int size, rsved_size;
} Codes;

Codes *new_codes()
{
    Codes *ret;

    ret = (Codes *)malloc(sizeof(Codes));
    assert(ret != NULL);
    ret->data = NULL;
    ret->size = 0;
    ret->rsved_size = 1;
    return ret;
}

void free_codes(Codes *this)
{
    int i;

    for (i = 0; i < this->size; i++) free(this->data[i]);
    free(this);
}

void codes_dump(Codes *this, FILE *fh)
{
    int i;

    for (i = 0; i < this->size; i++) {
        fputs(this->data[i], fh);
        fputc('\n', fh);
    }
}

void codes_append(Codes *this, const char *code)
{
    char *data;

    if (this->data == NULL || this->size == this->rsved_size) {
        this->data =
            (char **)realloc(this->data, sizeof(char *) * this->rsved_size * 2);
        assert(this->data != NULL);
        this->rsved_size *= 2;
    }

    data = (char *)malloc(sizeof(char) * (strlen(code) + 1));
    assert(data != NULL);
    strcpy(data, code);
    this->data[this->size++] = data;
}

void codes_appendf(Codes *this, const char *format, ...)
{
    va_list args;
    char buf[256]; /* TODO:should be enough??? */

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    codes_append(this, buf);
}

typedef struct {
    Codes *codes;
    int stack_idx, stack_max_idx;
    int nlabel;
} ObjEnv;

ObjEnv *new_objenv()
{
    ObjEnv *ret;

    ret = (ObjEnv *)malloc(sizeof(ObjEnv));
    assert(ret != NULL);
    ret->codes = new_codes();
    ret->stack_idx = ret->stack_max_idx = ret->nlabel = 0;
    return ret;
}

void free_objenv(ObjEnv *this)
{
    free_codes(this->codes);
    free(this);
}

Codes *objenv_swap_codes(ObjEnv *this, Codes *rhs)
{
    Codes *tmp;

    tmp = this->codes;
    this->codes = rhs;
    return tmp;
}

void write_obj_detail(AST *ast, ObjEnv *env)
{
    if (ast == NULL) return;

    switch (ast->kind) {
        case AST_PROG: {
            write_obj_detail(ast->stmt, env);
            write_obj_detail(ast->next, env);
            return;
        }

        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV: {
            char op[32];

            write_obj_detail(ast->rhs, env);
            codes_appendf(env->codes, "movss %%xmm0, -%d(%%rbp)",
                          ++env->stack_idx * 4);
            env->stack_max_idx = fmax(env->stack_max_idx, env->stack_idx);
            write_obj_detail(ast->lhs, env);

            switch (ast->kind) {
                case AST_ADD:
                    strcpy(op, "addss");
                    break;
                case AST_SUB:
                    strcpy(op, "subss");
                    break;
                case AST_MUL:
                    strcpy(op, "mulss");
                    break;
                case AST_DIV:
                    strcpy(op, "divss");
                    break;
            }

            codes_appendf(env->codes, "%s -%d(%%rbp), %%xmm0", op,
                          env->stack_idx-- * 4);

            return;
        }

        case AST_LITERAL: {
            if (ast->type.kind == TY_FLOAT) {
                codes_append(env->codes, ".data");
                codes_appendf(env->codes, ".L%d:", env->nlabel++);
                codes_appendf(env->codes, ".float %f", ast->fval);
                codes_append(env->codes, ".text");
                codes_appendf(env->codes, "movss .L%d(%%rip), %%xmm0",
                              env->nlabel - 1);
            }
            return;
        }
    }
}

void write_obj(AST *prog, FILE *fh)
{
    ObjEnv *env;
    Codes *header;

    assert(prog->kind == AST_PROG);

    env = new_objenv();

    codes_append(env->codes, ".data");
    codes_append(env->codes, "format:");
    codes_append(env->codes, ".string \"%f\\n\"");
    codes_append(env->codes, ".text");
    codes_append(env->codes, ".globl main");
    codes_append(env->codes, "main:");
    codes_append(env->codes, "push %rbp");
    codes_append(env->codes, "mov %rsp, %rbp");
    header = objenv_swap_codes(env, new_codes());

    write_obj_detail(prog, env);

    if (env->stack_max_idx > 0)
        codes_appendf(header, "sub $%d, %%rsp",
                      ((env->stack_max_idx / 16) + 1) * 16);

    codes_append(env->codes, "cvtss2sd %xmm0, %xmm0");
    codes_append(env->codes, "lea format(%rip), %rdi");
    codes_append(env->codes, "mov $1, %eax");
    codes_append(env->codes, "call printf");
    codes_append(env->codes, "leave");
    codes_append(env->codes, "ret");

    codes_dump(header, fh);
    codes_dump(env->codes, fh);

    free_codes(header);
    free_objenv(env);

    return;
}

#include "test.c"

int main(int argc, char **argv)
{
    Token *token_list;
    AST *prog;
    FILE *fh;

    if (argc == 1) {
        execute_test();
        return 0;
    }

    assert(argc == 3);
    fh = fopen(argv[1], "r");
    assert(fh != NULL);

    token_list = tokenize(fh);
    assert(token_list != NULL);
    fclose(fh);
    dump_token_list(token_list);

    prog = parse(token_list);

    fh = fopen(argv[2], "w");
    assert(fh != NULL);
    write_obj(prog, fh);
    fclose(fh);

    free_token_list(token_list);
    free_ast((AST *)prog);

    return 0;
}
