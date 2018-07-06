#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
long: 8-byte integer
double: 8-byte float
*/

#define true 1
#define false 0

int max(int lhs, int rhs) { return lhs < rhs ? rhs : lhs; }

enum {
    tINTEGER,
    tFLOAT,
    tLPAREN,
    tRPAREN,
    tVARIABLE,
    tPLUS,
    tMINUS,
    tSTAR,
    tSLASH,
    tSEMICOLON,
    tEOF,
};

typedef struct Token {
    int kind;

    union {
        double fval;
        long ival;
    };

    struct Token *next;
} Token;

enum {
    TY_LONG,
    TY_DOUBLE,
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
        double fval;
        long ival;

        /* AST_ADD, AST_SUB, AST_MUL, AST_DIV */
        struct {
            AST *lhs, *rhs;
        };
    };
};

Token *new_token(int kind);
Token *new_token_float(double num);
Token *tokenize(FILE *fp);
void free_token_list(Token *token_list);
AST *new_ast_float(double val);
AST *new_ast_integer(long val);
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

/********** Token *************/

Token *new_token(int kind)
{
    Token *token;

    token = (Token *)malloc(sizeof(Token));
    token->next = NULL;
    token->kind = kind;
    return token;
}

Token *new_token_float(double fval)
{
    Token *token = new_token(tFLOAT);

    token->fval = fval;
    return token;
}

Token *new_token_integer(long ival)
{
    Token *token = new_token(tINTEGER);

    token->ival = ival;
    return token;
}

enum {
    TK_ST_INITIAL,
    TK_ST_INTEGER,
    TK_ST_FLOAT,
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
                    buf[bufidx++] = ch;
                    st = TK_ST_INTEGER;
                    break;
                }

                if (ch == '.') {
                    bufidx = 0;
                    buf[bufidx++] = ch;
                    st = TK_ST_FLOAT;
                    break;
                }

                switch (ch) {
                    case '+':
                        token = new_token(tPLUS);
                        break;
                    case '-':
                        token = new_token(tMINUS);
                        break;
                    case '*':
                        token = new_token(tSTAR);
                        break;
                    case '/':
                        token = new_token(tSLASH);
                        break;
                    case '(':
                        token = new_token(tLPAREN);
                        break;
                    case ')':
                        token = new_token(tRPAREN);
                        break;
                    case ';':
                        token = new_token(tSEMICOLON);
                        break;
                }
                if (token != NULL) break;

                return NULL;

            case TK_ST_INTEGER:
                assert(bufidx < sizeof(buf) - 1);

                if (isdigit(ch) || ch == '.') {
                    buf[bufidx++] = ch;
                    if (ch == '.')
                        st = TK_ST_FLOAT; /* actually, it's a float. */
                    break;
                }

                buf[bufidx++] = '\0';
                token = new_token_integer(atol(buf));

                st = TK_ST_INITIAL;

                ungetc(ch, fp);
                break;

            case TK_ST_FLOAT:
                assert(bufidx < sizeof(buf) - 1);

                if (isdigit(ch) || ch == '.') {
                    buf[bufidx++] = ch;
                    break;
                }

                buf[bufidx++] = '\0';
                token = new_token_float(atof(buf));

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

    token = new_token(tEOF);
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

/******** AST *********/

AST *new_ast_float(double val)
{
    AST *ast;

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = AST_LITERAL;
    ast->type.kind = TY_DOUBLE;
    ast->fval = val;

    return ast;
}

AST *new_ast_integer(long val)
{
    AST *ast;

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = AST_LITERAL;
    ast->type.kind = TY_LONG;
    ast->ival = val;

    return ast;
}

AST *new_ast_binary_op(int kind, AST *lhs, AST *rhs)
{
    AST *ast;

    ast = (AST *)malloc(sizeof(AST));
    assert(ast != NULL);
    ast->kind = kind;
    ast->type.kind = lhs->type.kind == TY_DOUBLE || rhs->type.kind == TY_DOUBLE
                         ? TY_DOUBLE
                         : TY_LONG;
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

Token *pop_token_if(Token **token_list, int kind)
{
    Token *token = peek_token(token_list);

    if (token == NULL || token->kind != kind) return NULL;
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
    if (parse_match(token_list, tLPAREN) != NULL) {
        /* (expr) */
        AST *ast;

        ast = parse_expr(token_list);
        assert(parse_match(token_list, tRPAREN) != NULL);
        return ast;
    }

    /* number */
    Token *token;
    int minus = 1;

    token = pop_token_if(token_list, tMINUS);
    if (token != NULL) minus = -1;

    token = pop_token_if(token_list, tFLOAT);
    if (token != NULL) return new_ast_float(minus * token->fval);

    token = pop_token_if(token_list, tINTEGER);
    if (token != NULL) return new_ast_integer(minus * token->ival);

    return NULL;
}

AST *parse_term_detail(Token **token_list, AST *factor)
{
    Token *op;
    AST *ast = factor;

    if ((op = parse_match(token_list, tSTAR)) != NULL) {
        AST *lhs, *rhs;

        lhs = factor;
        rhs = parse_factor(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_MUL, lhs, rhs);
        ast = parse_term_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, tSLASH)) != NULL) {
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

    if ((op = parse_match(token_list, tPLUS)) != NULL) {
        AST *lhs, *rhs;

        dump_token_list(*token_list);

        lhs = term;
        rhs = parse_term(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_ADD, lhs, rhs);
        ast = parse_expr_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, tMINUS)) != NULL) {
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
            free_ast(ast->next);
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
            case tFLOAT:
                printf("%lff ", token->fval);
                break;

            case tINTEGER:
                printf("%ldi ", token->ival);
                break;

            case tPLUS:
                printf("+ ");
                break;

            case tMINUS:
                printf("- ");
                break;

            case tSTAR:
                printf("* ");
                break;

            case tSLASH:
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
            switch (ast->stmt->type.kind) {
                case TY_DOUBLE:
                    codes_append(env->codes, "lea doublefmt(%rip), %rdi");
                    break;

                case TY_LONG:
                    codes_append(env->codes, "mov %rax, %rsi");
                    codes_append(env->codes, "lea longfmt(%rip), %rdi");
                    break;
            }
            codes_append(env->codes, "mov $1, %eax");
            codes_append(env->codes, "call printf");

            write_obj_detail(ast->next, env);
            return;
        }

        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV: {
            char op[32];

            if (ast->lhs->type.kind == TY_LONG &&
                ast->rhs->type.kind == TY_LONG) {
                write_obj_detail(ast->rhs, env);
                codes_appendf(env->codes, "mov %%rax, -%d(%%rbp)",
                              ++env->stack_idx * 8);
                env->stack_max_idx = max(env->stack_max_idx, env->stack_idx);
                write_obj_detail(ast->lhs, env);
                if (ast->kind == AST_DIV) {
                    codes_append(env->codes, "cltd");
                    codes_appendf(env->codes, "idivq -%d(%%rbp)",
                                  env->stack_idx * 8);
                }
                else {
                    switch (ast->kind) {
                        case AST_ADD:
                            strcpy(op, "add");
                            break;
                        case AST_SUB:
                            strcpy(op, "sub");
                            break;
                        case AST_MUL:
                            strcpy(op, "imul");
                            break;
                    }
                    codes_appendf(env->codes, "%s -%d(%%rbp), %%rax", op,
                                  env->stack_idx * 8);
                }
                env->stack_idx--;
                break;
            }

            /* TODO: duplicate code */
            write_obj_detail(ast->rhs, env);
            if (ast->lhs->type.kind == TY_LONG &&
                ast->rhs->type.kind == TY_DOUBLE) {
                env->stack_idx += 2;
                codes_appendf(env->codes, "movsd %%xmm0, -%d(%%rbp)",
                              env->stack_idx * 8);
                env->stack_max_idx = max(env->stack_max_idx, env->stack_idx);
                write_obj_detail(ast->lhs, env);
                codes_append(env->codes, "cvtsi2sd %rax, %xmm0");
            }
            else if (ast->lhs->type.kind == TY_DOUBLE &&
                     ast->rhs->type.kind == TY_LONG) {
                codes_append(env->codes, "cvtsi2sd %rax, %xmm0");
                env->stack_idx += 2;
                codes_appendf(env->codes, "movsd %%xmm0, -%d(%%rbp)",
                              env->stack_idx * 8);
                env->stack_max_idx = max(env->stack_max_idx, env->stack_idx);
                write_obj_detail(ast->lhs, env);
            }
            else {
                assert(ast->lhs->type.kind == TY_DOUBLE &&
                       ast->rhs->type.kind == TY_DOUBLE);

                env->stack_idx += 2;
                codes_appendf(env->codes, "movsd %%xmm0, -%d(%%rbp)",
                              env->stack_idx * 8);
                env->stack_max_idx = max(env->stack_max_idx, env->stack_idx);
                write_obj_detail(ast->lhs, env);
            }

            switch (ast->kind) {
                case AST_ADD:
                    strcpy(op, "addsd");
                    break;
                case AST_SUB:
                    strcpy(op, "subsd");
                    break;
                case AST_MUL:
                    strcpy(op, "mulsd");
                    break;
                case AST_DIV:
                    strcpy(op, "divsd");
                    break;
            }

            codes_appendf(env->codes, "%s -%d(%%rbp), %%xmm0", op,
                          env->stack_idx * 8);
            env->stack_idx -= 2;

            return;
        }

        case AST_LITERAL: {
            switch (ast->type.kind) {
                case TY_DOUBLE:
                    codes_append(env->codes, ".data");
                    codes_appendf(env->codes, ".L%d:", env->nlabel++);
                    codes_appendf(env->codes, ".double %lf", ast->fval);
                    codes_append(env->codes, ".text");
                    codes_appendf(env->codes, "movsd .L%d(%%rip), %%xmm0",
                                  env->nlabel - 1);
                    break;

                case TY_LONG:
                    codes_appendf(env->codes, "mov $%ld, %%rax", ast->ival);
                    break;
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
    codes_append(env->codes, "doublefmt:");
    codes_append(env->codes, ".string \"%lff\\n\"");
    codes_append(env->codes, "longfmt:");
    codes_append(env->codes, ".string \"%ldi\\n\"");
    codes_append(env->codes, ".text");
    codes_append(env->codes, ".globl main");
    codes_append(env->codes, "main:");
    codes_append(env->codes, "push %rbp");
    codes_append(env->codes, "mov %rsp, %rbp");
    header = objenv_swap_codes(env, new_codes());

    write_obj_detail(prog, env);

    if (env->stack_max_idx > 0)
        codes_appendf(header, "sub $%d, %%rsp",
                      ((env->stack_max_idx * 8 / 16) + 1) * 16);
    codes_append(env->codes, "mov $0, %eax");
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
