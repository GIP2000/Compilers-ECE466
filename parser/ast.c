#include "./ast.h"
#include "../parser.tab.h"
#include "symbol_table.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AstNode *make_AstNode(int type) {
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    node->type = type;
    return node;
}

AstNode *make_binary_op(int op, AstNode *left, AstNode *right) {
    AstNode *ast = make_AstNode(ASTNODE_BINARYOP);
    struct BinaryOp *bo = &ast->binary_op;
    bo->op = op;
    bo->left = left;
    bo->right = right;
    return ast;
}

AstNode *make_unary_op(int op, AstNode *child) {
    AstNode *ast = make_AstNode(ASTNODE_UNARYOP);
    struct UnaryOp *uo = &ast->unary_op;
    uo->op = op;
    uo->child = child;
    return ast;
}

AstNode *make_IdentNode(YYlvalStrLit val) {
    AstNode *ast = make_AstNode(ASTNODE_IDENT);
    ast->ident = val.str;
    return ast;
}

AstNode *make_ternary_op(AstNode *cond, AstNode *truthy, AstNode *falsey) {
    AstNode *ast = make_AstNode(ASTNODE_TERNAYROP);
    ast->ternary_op.cond = cond;
    ast->ternary_op.truthy = truthy;
    ast->ternary_op.falsey = falsey;
    return ast;
}

AstNode *make_func_call(AstNode *name, struct AstNodeListNode *arguments) {
    AstNode *ast = make_AstNode(ASTNODE_FUNCCALL);
    ast->func_call.name = name;
    if (arguments == NULL) {
        ast->func_call.argument_count = 0;
        return ast;
    }
    struct AstNodeListNode *prev = NULL;
    struct AstNodeListNode *next = NULL;
    struct AstNodeListNode *current;
    int i;
    for (i = 0, current = arguments; current != NULL; ++i, current = next) {
        next = current->next;
        current->next = prev;
        prev = current;
    }
    ast->func_call.arguments = *prev;
    free(prev);

    ast->func_call.argument_count = i;

    return ast;
}

struct DeclaratorListNode *make_DeclaratorListNode(struct SymbolTableNode *n) {
    struct DeclaratorListNode *node =
        (struct DeclaratorListNode *)malloc(sizeof(struct DeclaratorListNode));
    node->node = n;
    return node;
}

AstNode *make_DeclaratorList(struct SymbolTableNode *n) {
    AstNode *ast = make_AstNode(ASTNODE_DECLARATORLIST);
    struct DeclaratorListNode *node = make_DeclaratorListNode(n);
    ast->declarator_list.head = node;
    ast->declarator_list.tail = node;
    return ast;
}
void append_DeclaratorList(AstNode *declarator_list,
                           struct SymbolTableNode *n) {
    struct DeclaratorListNode *node = make_DeclaratorListNode(n);
    declarator_list->declarator_list.tail->next = node;
    declarator_list->declarator_list.tail = node;
}

struct AstNodeListNode *make_node_list_node(AstNode *node) {
    struct AstNodeListNode *head =
        (struct AstNodeListNode *)malloc(sizeof(struct AstNodeListNode));
    head->val = node;
    head->next = NULL;
    return head;
}

struct AstNodeListNode *append_AstNodeListNode(struct AstNodeListNode *node,
                                               AstNode *next) {
    struct AstNodeListNode *head = make_node_list_node(next);
    head->next = node;
    return head;
}

void get_op_str(char *out_str, int op) {
    switch (op) {
    case PLUSPLUS:
        out_str[0] = '+';
        out_str[1] = '+';
        break;
    case MINUSMINUS:
        out_str[0] = '-';
        out_str[1] = '-';
        break;
    case SHL:
        out_str[0] = '<';
        out_str[1] = '<';
        break;
    case SHR:
        out_str[0] = '>';
        out_str[1] = '>';
        break;
    case LTEQ:
        out_str[0] = '<';
        out_str[1] = '=';
        break;
    case GTEQ:
        out_str[0] = '>';
        out_str[1] = '=';
        break;
    case EQEQ:
        out_str[0] = '=';
        out_str[1] = '=';
        break;
    case NOTEQ:
        out_str[0] = '!';
        out_str[1] = '=';
        break;
    case LOGAND:
        out_str[0] = '&';
        out_str[1] = '&';
        break;
    case LOGOR:
        out_str[0] = '|';
        out_str[1] = '|';
        break;
    case ELLIPSIS:
        out_str[0] = '.';
        out_str[1] = '.';
        out_str[2] = '.';
        break;
    case TIMESEQ:
        out_str[0] = '*';
        out_str[1] = '=';
        break;
    case DIVEQ:
        out_str[0] = '/';
        out_str[1] = '=';
        break;
    case MODEQ:
        out_str[0] = '%';
        out_str[1] = '=';
        break;
    case PLUSEQ:
        out_str[0] = '+';
        out_str[1] = '=';
        break;
    case MINUSEQ:
        out_str[0] = '-';
        out_str[1] = '=';
        break;
    case SHLEQ:
        out_str[0] = '<';
        out_str[1] = '<';
        out_str[2] = '=';
        break;
    case SHREQ:
        out_str[0] = '>';
        out_str[1] = '>';
        out_str[2] = '=';
        break;
    case ANDEQ:
        out_str[0] = '&';
        out_str[1] = '=';
        break;
    case OREQ:
        out_str[0] = '|';
        out_str[1] = '=';
        break;
    case XOREQ:
        out_str[0] = '^';
        out_str[1] = '=';
        break;
    case SIZEOF:
        out_str[0] = 'S';
        out_str[1] = 'Z';
        out_str[2] = 'O';
        break;
    default: {
        if (op == '!' || op == '%' || op == '&' || (op >= '*' && op <= '/') ||
            (op >= '<' && op <= '>') || op == '^' || op == '~' || op == '|') {
            out_str[0] = (char)op;
        } else {
            fprintf(stderr, "Invalid Operator");
        }
    }
    }
}

void add_tab(unsigned int tab_count) {
    unsigned int i;
    for (i = 0; i < tab_count; ++i) {
        printf("  ");
    }
}

// print in a dfs fasion
void print_AstNode(AstNode *head, unsigned int tab_count) {
    if (head == NULL) {
        printf("NULL founnd\n");
        return;
    }
    // make the tabs
    add_tab(tab_count);
    switch (head->type) {
    case ASTNODE_CONSTANT:
        printf("Constant ");
        if (head->constant.type < 3) {
            printf("%lld", (long long)head->constant.val.u_int);
        } else if (head->constant.type < 6) {
            printf("%lld", head->constant.val.u_int);
        } else if (head->constant.type < 9) {
            printf("%Lg", head->constant.val.flt);
        } else {
            printf("%c", head->constant.val.chr);
        }
        printf("\n");
        return;
    case ASTNODE_STRLIT:
        printf("String Literal %s\n", head->strlit.str);
        return;
    case ASTNODE_IDENT:
        printf("IDENT %s\n", head->ident);
        return;
    case ASTNODE_UNARYOP: {
        char op[4] = {0, 0, 0, 0};
        get_op_str(op, head->unary_op.op);
        printf("Unary OP %s\n", op);
        return print_AstNode(head->unary_op.child, tab_count + 1);
    }
    case ASTNODE_BINARYOP: {
        char op[4] = {0, 0, 0, 0};
        get_op_str(op, head->binary_op.op);
        printf("BINARY OP %s\n", op);
        print_AstNode(head->binary_op.left, tab_count + 1);
        return print_AstNode(head->binary_op.right, tab_count + 1);
    }
    case ASTNODE_TERNAYROP:
        printf("Ternary OP, IF\n");
        print_AstNode(head->ternary_op.cond, tab_count + 1);
        add_tab(tab_count);
        printf("Then:\n");
        print_AstNode(head->ternary_op.truthy, tab_count + 1);
        add_tab(tab_count);
        printf("Else:\n");
        return print_AstNode(head->ternary_op.falsey, tab_count + 1);
    case ASTNODE_FUNCCALL: {
        printf("Func Call, %d arguments\n", head->func_call.argument_count);
        print_AstNode(head->func_call.name, tab_count + 1);
        int i;
        struct AstNodeListNode *arg;
        for (i = 1, arg = &head->func_call.arguments; arg != NULL;
             arg = arg->next, ++i) {
            add_tab(tab_count);
            printf("arg #%d\n", i);
            print_AstNode(arg->val, tab_count + 1);
        }
        return;
    }
    default:
        fprintf(stderr, "Unsuportted Node type %d\n", head->type);
        exit(1);
    }
}
