#include "./ast.h"
#include <stdio.h>
#include <stdlib.h>

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
    // TODO reverse the linked list
    struct AstNodeListNode *tail = arguments;
    int i;
    for (i = 1; tail != NULL; ++i, tail = tail->next) {
    }
    ast->func_call.argument_count = i;

    return ast;
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
    case ASTNODE_UNARYOP:
        printf("UNARY OP%c(%d)\n", (char)head->unary_op.op, head->unary_op.op);
        return print_AstNode(head->unary_op.child, tab_count + 1);
    case ASTNODE_BINARYOP:
        printf("BINARY OP %c\n", (char)head->binary_op.op);
        print_AstNode(head->binary_op.left, tab_count + 1);
        return print_AstNode(head->binary_op.right, tab_count + 1);
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
        fprintf(stderr, "Unsuportted Node type\n");
        exit(1);
    }
}
