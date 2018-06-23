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

#include "test.c"

int main(void)
{
    Token *token_list;
    ASTProg *prog;

    execute_test();

    while (1) {
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

        free_token_list(token_list);
        free_ast((AST *)prog);
    }

    return 0;
}
