#include "./ast.h"
#include "../lexer/file_info.h"
#include "../parser.tab.h"
#include "symbol_table.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FileInfo file_info;
extern int yylineno;

extern struct SymbolTable *symbol_table;

AstNode *make_AstNode(int type) {
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    node->type = type;
    node->fi.file_name = file_info.file_name;
    node->fi.ln =
        yylineno - file_info.real_line_start + file_info.file_line_start;
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
    if (!find_in_namespace(val.str, ANY, &ast->ident)) {
        fprintf(stderr, "Ident: %s\n", val.str);
        yyerror("Ident referenced before assignment");
        exit(2);
    };
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

struct StatmentListNode *make_StatmentListNode(AstNode *n) {
    struct StatmentListNode *node =
        (struct StatmentListNode *)malloc(sizeof(struct StatmentListNode));
    node->node = n;
    node->next = NULL;
    return node;
}

AstNode *make_StatementList(AstNode *node) {
    AstNode *ast = make_AstNode(ASTNODE_STATMENTLIST);
    struct StatmentListNode *list_node = make_StatmentListNode(node);
    ast->statments.head = list_node;
    ast->statments.tail = list_node;
    return ast;
}

void append_StatmentList(struct StatmentList *statment_list,
                         struct AstNode *next) {
    struct StatmentListNode *list_node = make_StatmentListNode(next);
    statment_list->tail->next = list_node;
    statment_list->tail = list_node;
}

struct AstNodeListNode *make_node_list_node(AstNode *node) {
    struct AstNodeListNode *head =
        (struct AstNodeListNode *)malloc(sizeof(struct AstNodeListNode));
    head->val = node;
    head->next = NULL;
    return head;
}
AstNode *make_Declaration(struct SymbolTableNode *symbol) {
    AstNode *ast = make_AstNode(ASTNODE_DECLARATION);
    ast->declaration.symbol = symbol;
    return ast;
}

AstNode *make_IfStatment(AstNode *cond, AstNode *statment,
                         AstNode *else_statment) {
    AstNode *node = make_AstNode(ASTNODE_IF_STATMENT);
    node->if_statment.cmp = cond;
    node->if_statment.statment = statment;
    node->if_statment.else_statment = else_statment;
    return node;
}

AstNode *make_WhileStatment(AstNode *cond, AstNode *statment, int is_do) {
    AstNode *node = make_AstNode(ASTNODE_WHILE_STATMENT);
    node->while_statment.cmp = cond;
    node->while_statment.statment = statment;
    node->while_statment.is_do = is_do;
    return node;
}
AstNode *make_ForStatment(AstNode *initalizer, AstNode *cond,
                          AstNode *incrementer, AstNode *statment) {
    AstNode *node = make_AstNode(ASTNODE_FOR_STATMENT);
    node->for_statment.initalizer = initalizer;
    node->for_statment.cmp = cond;
    node->for_statment.incrementer = incrementer;
    node->for_statment.statment = statment;
    return node;
}
AstNode *make_GotoStatment(struct SymbolTableNode *node) {
    AstNode *ast = make_AstNode(ASTNODE_GOTO_STATMENT);
    ast->goto_statment = node;
    return ast;
}

AstNode *make_LabelStatment(struct SymbolTableNode *node) {

    AstNode *ast = make_AstNode(ASTNODE_LABEL_STATMENT);
    ast->goto_statment = node;
    return ast;
}
AstNode *make_ReturnStatment(AstNode *statment) {
    AstNode *ast = make_AstNode(ASTNODE_RETURN_STATMENT);
    ast->return_statment = statment;
    return ast;
}
AstNode *make_SwitchStatment(AstNode *cmp, AstNode *statment) {
    AstNode *ast = make_AstNode(ASTNODE_SWITCH_STATMENT);
    ast->switch_statment.cmp = cmp;
    ast->switch_statment.statment = statment;
    return ast;
}
AstNode *make_CaseStatment(AstNode *cmp, AstNode *statment) {
    AstNode *ast = make_AstNode(ASTNODE_CASE_STATMENT);
    ast->case_statment.cmp = cmp;
    ast->case_statment.statment = statment;
    return ast;
}
AstNode *make_DefaultStatment(AstNode *statment) {
    AstNode *ast = make_AstNode(ASTNODE_DEFAULT_STATMENT);
    ast->deafult_statment = statment;
    return ast;
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
        printf("NULL found\n");
        return;
    }
    // make the tabs
    add_tab(tab_count);
    printf("%s:%d: ", head->fi.file_name, head->fi.ln);
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
        printf("IDENT %s with type: \n", head->ident->name);
        add_tab(tab_count + 1);
        print_type(head->ident->val.type);
        printf("\n");
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
    case ASTNODE_STATMENTLIST: {
        printf("StatmentList: \n");
        struct StatmentListNode *start;
        int i;
        for (i = 0, start = head->statments.head; start != NULL;
             start = start->next, ++i) {
            printf("statment #%d: \n", i);
            print_AstNode(start->node, tab_count + 1);
            if (start->next != NULL) {
                add_tab(tab_count);
            }
        }
        return;
    }
    case ASTNODE_DECLARATION:
        printf(
            "Decleration: %s namespace (%d) ident_type (%d) sc (%d) type: ",
            head->declaration.symbol->name, head->declaration.symbol->namespc,
            head->declaration.symbol->type, head->declaration.symbol->val.sc);
        print_type(head->declaration.symbol->val.type);
        printf("\n");
        return;
    case ASTNODE_IF_STATMENT:
        printf("IF Statment: Condition: \n");
        print_AstNode(head->if_statment.cmp, tab_count + 1);
        add_tab(tab_count);
        printf("Then: \n");
        print_AstNode(head->if_statment.statment, tab_count + 1);
        add_tab(tab_count);
        if (head->if_statment.else_statment != NULL) {
            printf("Otherwise: \n");
            print_AstNode(head->if_statment.else_statment, tab_count + 1);
        }
        return;
    case ASTNODE_WHILE_STATMENT:
        printf("%sWhile Statment: Comparison: ",
               head->while_statment.is_do ? "DO " : "");

        print_AstNode(head->while_statment.cmp, tab_count + 1);
        add_tab(tab_count);
        printf("Loop Statment: ");
        print_AstNode(head->while_statment.statment, tab_count);
        return;
    case ASTNODE_FOR_STATMENT:
        printf("For Loop Statment: Initalizer: ");

        print_AstNode(head->for_statment.initalizer, tab_count + 1);
        add_tab(tab_count);
        printf("Comparison: ");
        print_AstNode(head->for_statment.cmp, tab_count);
        add_tab(tab_count);
        printf("Incrementer: ");
        print_AstNode(head->for_statment.incrementer, tab_count);
        add_tab(tab_count);
        printf("Statment: ");
        print_AstNode(head->for_statment.statment, tab_count);
        return;
    case ASTNODE_GOTO_STATMENT:
        printf("Goto %s\n", head->goto_statment->name);
        return;
    case ASTNODE_CONTINUE_STATMENT:
        printf("Continue\n");
        return;
    case ASTNODE_BREAK_STATMENT:
        printf("Break\n");
        return;
    case ASTNODE_RETURN_STATMENT:
        printf("Return: \n");
        print_AstNode(head->return_statment, tab_count + 1);
        return;
    case ASTNODE_LABEL_STATMENT:
        printf("Label %s\n", head->goto_statment->name);
        return;
    case ASTNODE_SWITCH_STATMENT:
        printf("Switch Conditon: \n");
        print_AstNode(head->switch_statment.cmp, tab_count + 1);
        add_tab(tab_count);
        printf("Statment: \n");
        print_AstNode(head->switch_statment.statment, tab_count);
        return;
    case ASTNODE_CASE_STATMENT:
        printf("Case Statment Condition: \n");
        print_AstNode(head->case_statment.cmp, tab_count + 1);
        add_tab(tab_count);
        printf("Statment: \n");
        print_AstNode(head->case_statment.statment, tab_count + 1);
        return;
    case ASTNODE_DEFAULT_STATMENT:
        printf("Deafult Statment :\n");
        print_AstNode(head->deafult_statment, tab_count + 1);
        return;
    default:
        fprintf(stderr, "Unsuportted Node type %d\n", head->type);
        exit(1);
    }
}
