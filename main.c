#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0

float fmax(float lhs, float rhs) { return lhs < rhs ? rhs : lhs; }

typedef enum TOKEN_KIND TOKEN_KIND;
typedef struct Token Token;

enum {
    TOKEN_NUMERIC,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_VARIABLE,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_SEMICOLON,
};

struct Token {
    union {
        float num;
        char str[10];
    } data;
    int kind;

    Token *next;
};

enum { AST_NUMERIC, AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_PROG };

typedef struct AST AST;
typedef struct ASTNumeric ASTNumeric;
typedef struct ASTBinaryOp ASTBinaryOp;
typedef struct ASTProg ASTProg;

struct AST {
    int kind;
};

struct ASTProg {
    AST super;
    AST *stmt;
    ASTProg *next;
};

struct ASTNumeric {
    AST super;
    float num;
};

struct ASTBinaryOp {
    AST super;
    AST *lhs, *rhs;
};

Token *new_simple_token(int kind);
Token *new_numeric_token(float num);
Token *tokenize(FILE *fp, int eval);
void free_token_list(Token *token_list);
ASTNumeric *new_ast_numeric(float num);
ASTBinaryOp *new_ast_binary_op(int kind, AST *lhs, AST *rhs);
Token *pop_token(Token **token_list);
Token *peek_token(Token **token_list);
Token *pop_numeric_token(Token **token_list);
Token *parse_match(Token **token_list, int kind);
AST *parse_factor(Token **token_list);
AST *parse_term_detail(Token **token_list, AST *factor);
AST *parse_term(Token **token_list);
AST *parse_expr_detail(Token **token_list, AST *term);
AST *parse_expr(Token **token_list);
ASTProg *parse_prog(Token **token_list);
ASTProg *parse(Token *token_list);
void dump_token_list(Token *token_list);

Token *new_simple_token(int kind)
{
    Token *token;

    token = (Token *)malloc(sizeof(Token));
    token->next = NULL;
    token->kind = kind;
    return token;
}

Token *new_numeric_token(float num)
{
    Token *token = new_simple_token(TOKEN_NUMERIC);

    token->data.num = num;
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
                switch (ch) {
                    case '+':
                        token = new_simple_token(TOKEN_ADD);
                        break;
                    case '-':
                        token = new_simple_token(TOKEN_SUB);
                        break;
                    case '*':
                        token = new_simple_token(TOKEN_MUL);
                        break;
                    case '/':
                        token = new_simple_token(TOKEN_DIV);
                        break;
                    case '(':
                        token = new_simple_token(TOKEN_LPAREN);
                        break;
                    case ')':
                        token = new_simple_token(TOKEN_RPAREN);
                        break;
                    case ';':
                        token = new_simple_token(TOKEN_SEMICOLON);
                        break;
                }
                if (token != NULL) break;

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

ASTNumeric *new_ast_numeric(float num)
{
    ASTNumeric *ast;

    ast = (ASTNumeric *)malloc(sizeof(ASTNumeric));
    assert(ast != NULL);
    ast->super.kind = AST_NUMERIC;
    ast->num = num;

    return ast;
}

ASTBinaryOp *new_ast_binary_op(int kind, AST *lhs, AST *rhs)
{
    ASTBinaryOp *ast;

    ast = (ASTBinaryOp *)malloc(sizeof(ASTBinaryOp));
    assert(ast != NULL);
    ast->super.kind = kind;
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

Token *pop_numeric_token(Token **token_list)
{
    Token *token = peek_token(token_list);

    if (token == NULL || token->kind != TOKEN_NUMERIC) return NULL;
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

    if (parse_match(token_list, TOKEN_LPAREN) != NULL) {
        /* (expr) */
        ast = parse_expr(token_list);
        assert(parse_match(token_list, TOKEN_RPAREN) != NULL);
    }
    else {
        /* numeric */
        Token *token;

        token = pop_numeric_token(token_list);
        if (token == NULL) return NULL;

        dump_token_list(*token_list);

        ast = (AST *)new_ast_numeric(token->data.num);
    }

    return ast;
}

AST *parse_term_detail(Token **token_list, AST *factor)
{
    Token *op;
    AST *ast = factor;

    if ((op = parse_match(token_list, TOKEN_MUL)) != NULL) {
        AST *lhs, *rhs;

        lhs = factor;
        rhs = parse_factor(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_MUL, lhs, rhs);
        ast = parse_term_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, TOKEN_DIV)) != NULL) {
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

    if ((op = parse_match(token_list, TOKEN_ADD)) != NULL) {
        AST *lhs, *rhs;

        dump_token_list(*token_list);

        lhs = term;
        rhs = parse_term(token_list);
        assert(rhs != NULL);
        ast = (AST *)new_ast_binary_op(AST_ADD, lhs, rhs);
        ast = parse_expr_detail(token_list, ast);
    }
    else if ((op = parse_match(token_list, TOKEN_SUB)) != NULL) {
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
    assert(parse_match(token_list, TOKEN_SEMICOLON) != NULL);
    dump_token_list(*token_list);

    return ast;
}

ASTProg *parse_prog(Token **token_list)
{
    AST *stmt;
    ASTProg *prog, *ast;

    stmt = parse_stmt(token_list);
    if (stmt == NULL) return NULL;
    prog = parse_prog(token_list);

    ast = (ASTProg *)malloc(sizeof(ASTProg));
    assert(ast != NULL);
    ast->super.kind = AST_PROG;
    ast->stmt = stmt;
    ast->next = prog;

    return ast;
}

ASTProg *parse(Token *token_list)
{
    ASTProg *prog;

    prog = parse_prog(&token_list);
    assert(token_list == NULL);

    return prog;
}

float eval_ast(AST *ast)
{
    switch (ast->kind) {
        case AST_PROG: {
            ASTProg *prog = (ASTProg *)ast;
            while (prog != NULL) {
                printf("%f\n", eval_ast(prog->stmt));
                prog = prog->next;
            }
            return 0;
        }

        case AST_ADD: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            return eval_ast(bin->lhs) + eval_ast(bin->rhs);
        }

        case AST_SUB: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            return eval_ast(bin->lhs) - eval_ast(bin->rhs);
        }

        case AST_MUL: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            return eval_ast(bin->lhs) * eval_ast(bin->rhs);
        }

        case AST_DIV: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            return eval_ast(bin->lhs) / eval_ast(bin->rhs);
        }

        case AST_NUMERIC: {
            ASTNumeric *num = (ASTNumeric *)ast;
            return num->num;
        }
    }

    assert(false);
}

void free_ast(AST *ast)
{
    if (ast == NULL) return;

    switch (ast->kind) {
        case AST_PROG: {
            ASTProg *prog = (ASTProg *)ast;
            free_ast(prog->stmt);
            free_ast((AST *)prog->next);
            free(prog);
            return;
        }

        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            free_ast(bin->lhs);
            free_ast(bin->rhs);
            free(bin);
            return;
        }

        case AST_NUMERIC: {
            ASTNumeric *num = (ASTNumeric *)ast;
            free(num);
            return;
        }
    }

    assert(false);
}

void dump_token_list(Token *token)
{
    for (; token != NULL; token = token->next) {
        switch (token->kind) {
            case TOKEN_NUMERIC:
                printf("%f ", token->data.num);
                break;

            case TOKEN_ADD:
                printf("+ ");
                break;

            case TOKEN_SUB:
                printf("- ");
                break;

            case TOKEN_MUL:
                printf("* ");
                break;

            case TOKEN_DIV:
                printf("/ ");
                break;

            case TOKEN_LPAREN:
                printf("( ");
                break;

            case TOKEN_RPAREN:
                printf(") ");
                break;

            case TOKEN_SEMICOLON:
                printf(";\n");
                break;

            default:
                printf("???%d\n", token->kind);
                assert(false);
        }
    }

    puts("");
}

typedef struct Codes Codes;
typedef struct ObjEnv ObjEnv;

struct Codes {
    char **data;
    int size, rsved_size;
};

Codes *new_codes()
{
    Codes *ret;

    ret = (Codes *)malloc(sizeof(Codes));
    assert(ret != NULL);
    ret->data = NULL;
    ret->size = 0;
    ret->rsved_size = 10;
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

struct ObjEnv {
    Codes *codes;
    int stack_idx, stack_max_idx;
    int nlabel;
};

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

void append_code(ObjEnv *env, const char *code)
{
    codes_append(env->codes, code);
}

void write_obj_detail(AST *ast, ObjEnv *env)
{
    if (ast == NULL) return;

    switch (ast->kind) {
        case AST_PROG: {
            ASTProg *prog = (ASTProg *)ast;
            write_obj_detail(prog->stmt, env);
            write_obj_detail((AST *)prog->next, env);
            return;
        }

        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV: {
            ASTBinaryOp *bin = (ASTBinaryOp *)ast;
            char op[32], buf[256];

            write_obj_detail(bin->rhs, env);
            sprintf(buf, "movss %%xmm0, -%d(%%rbp)", ++env->stack_idx * 4);
            append_code(env, buf);
            env->stack_max_idx = fmax(env->stack_max_idx, env->stack_idx);
            write_obj_detail(bin->lhs, env);

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

            sprintf(buf, "%s -%d(%%rbp), %%xmm0", op, env->stack_idx-- * 4);
            append_code(env, buf);

            return;
        }

        case AST_NUMERIC: {
            ASTNumeric *num = (ASTNumeric *)ast;
            char buf[256];

            append_code(env, ".data");
            sprintf(buf, ".L%d:", env->nlabel++);
            append_code(env, buf);
            sprintf(buf, ".float %f", num->num);
            append_code(env, buf);
            append_code(env, ".text");
            sprintf(buf, "movss .L%d(%%rip), %%xmm0", env->nlabel - 1);
            append_code(env, buf);
            return;
        }
    }
}

void write_obj(ASTProg *prog, FILE *fh)
{
    ObjEnv *env;
    Codes *header;
    char buf[256];

    env = new_objenv();

    append_code(env, ".data");
    append_code(env, "format:");
    append_code(env, ".string \"%f\\n\"");
    append_code(env, ".text");
    append_code(env, ".globl main");
    append_code(env, "main:");
    append_code(env, "push %rbp");
    append_code(env, "mov %rsp, %rbp");
    header = objenv_swap_codes(env, new_codes());

    write_obj_detail((AST *)prog, env);

    if (env->stack_max_idx > 0) {
        sprintf(buf, "sub $%d, %%rsp", ((env->stack_max_idx / 16) + 1) * 16);
        codes_append(header, buf);
    }

    append_code(env, "cvtss2sd %xmm0, %xmm0");
    append_code(env, "lea format(%rip), %rdi");
    append_code(env, "call printf");
    append_code(env, "leave");
    append_code(env, "ret");

    codes_dump(header, fh);
    codes_dump(env->codes, fh);

    free_codes(header);
    free_objenv(env);

    return;
}

#include "test.c"

int main(void)
{
    execute_test();

    while (1) {
        Token *token_list;
        ASTProg *prog;
        FILE *fh;

        token_list = tokenize(stdin, 1);
        if (token_list == NULL) {
            printf("Error: can't tokenize.\n");
            return 1;
        }
        dump_token_list(token_list);

        prog = parse(token_list);
        printf("debug\n");
        fflush(stdout);
        eval_ast((AST *)prog);

        fh = fopen("outout.s", "w");
        assert(fh != NULL);
        write_obj(prog, fh);
        fclose(fh);

        free_token_list(token_list);
        free_ast((AST *)prog);
    }

    return 0;
}
