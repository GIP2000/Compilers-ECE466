#include "./ast.h"
#include "../lexer/file_info.h"
#include "../macro_util.h"
#include "../parser.tab.h"
#include "symbol_table.h"
#include "yylval_types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FileInfo file_info;
extern int yylineno;
extern struct SymbolTable *symbol_table;

struct Type TTUCHAR;
struct Type TTULONG;
int initalized = 0;
int initalized_str = 0;

#define INITALIZE(init, type_name, next_type_val)                              \
    (init).qualifier_bit_mask = 0;                                             \
    (init).type = (type_name);                                                 \
    (init).extentions.next_type.next = (next_type_val);

void promote_arr(AstNode *arr) {
    if (arr->value_type->type == T_ARR) {
        struct Type *ptr = make_next_type(
            T_POINTER, arr->value_type->extentions.next_type.next);
        arr->value_type = ptr;
    }
}

AstNode *make_AstNode(int type) {
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    node->type = type;
    node->value_type = NULL;
    node->fi.file_name = file_info.file_name;
    node->fi.ln =
        yylineno - file_info.real_line_start + file_info.file_line_start;
    return node;
}

AstNode *make_ConstantType(YYlvalNumLit numlit) {
    static struct Type TTINT;
    static struct Type TTLONG;
    static struct Type TTLONGLONG;
    static struct Type TTUINT;
    static struct Type TTULONGLONG;
    static struct Type TTDOUBLE;
    static struct Type TTFLOAT;
    static struct Type TTLONGDOUBLE;

    AstNode *ast = make_AstNode(ASTNODE_CONSTANT);
    ast->constant = numlit;

    if (!initalized) {
        initalized = 1;
        INITALIZE(TTINT, T_INT, NULL)
        INITALIZE(TTLONG, T_LONG, &TTINT)
        INITALIZE(TTLONGLONG, T_LONG, &TTLONG)
        INITALIZE(TTUINT, T_UNSIGNED, &TTINT)
        INITALIZE(TTULONG, T_LONG, &TTUINT)
        INITALIZE(TTULONGLONG, T_LONG, &TTULONG)
        INITALIZE(TTDOUBLE, T_DOUBLE, NULL)
        INITALIZE(TTFLOAT, T_FLOAT, NULL)
        INITALIZE(TTLONGDOUBLE, T_LONG, &TTDOUBLE)
        INITALIZE(TTUCHAR, T_CHAR, NULL)
    }

    switch (numlit.type) {
    case TINT:
        ast->value_type = &TTINT;
        break;
    case TLONG:
        ast->value_type = &TTLONG;
        break;
    case TLONGLONG:
        ast->value_type = &TTLONGLONG;
        break;
    case TUINT:
        ast->value_type = &TTUINT;
        break;
    case TULONG:
        ast->value_type = &TTULONG;
        break;
    case TULONGLONG:
        ast->value_type = &TTULONGLONG;
        break;
    case TDOUBLE:
        ast->value_type = &TTDOUBLE;
        break;
    case TFLOAT:
        ast->value_type = &TTFLOAT;
        break;
    case TLONGDOUBLE:
        ast->value_type = &TTLONGDOUBLE;
        break;
    case TUCHAR:
        ast->value_type = &TTUCHAR;
        break;
    default:
        fprintf(stderr, "UNREACHABLE\n");
        exit(1);
    }
    return ast;
}

AstNode *make_StringType(YYlvalStrLit strlit) {
    static struct Type TTSTR;
    if (initalized_str) {
        INITALIZE(TTSTR, T_POINTER, &TTUCHAR)
    }
    AstNode *ast = make_AstNode(ASTNODE_STRLIT);
    ast->strlit = strlit;
    ast->value_type = &TTSTR;
    return ast;
}

int type_can_do_math(struct Type *t) {
    return t->type > T_LABEL && t->type <= T_LONG;
}

int get_long_len(struct Type *t, int *has_unsigned) {
    struct Type *cur;
    int i;
    int has_unsigned_l = 0;
    int signed_count = 0;
    for (cur = t, i = 0;
         cur != NULL && cur->type > T_POINTER && cur->type <= T_LONG;
         ++i, cur = cur->extentions.next_type.next) {

        if (cur->type == T_UNSIGNED) {
            has_unsigned_l = 1;
        }
        if (cur->type == T_SIGNED) {
            signed_count++;
        }
    }
    PTR_RETURN(has_unsigned, has_unsigned_l)
    return i - signed_count;
}

int types_are_eq(struct Type *left, struct Type *right) {
    struct Type *cur_left;
    struct Type *cur_right;
    for (cur_left = left, cur_right = right;
         (cur_left != NULL && cur_left->type >= T_POINTER &&
          cur_left->type <= T_TYPEDEF) &&
         (cur_right != NULL && cur_right->type >= T_POINTER &&
          cur_right->type <= T_TYPEDEF);
         cur_left = cur_left->extentions.next_type.next,
        cur_right = cur_right->extentions.next_type.next) {
        if (cur_left->type != cur_right->type) {
            return 0;
        }
    }
    if (cur_right == NULL && cur_left == NULL)
        return 1;
    if (cur_right != NULL && cur_left != NULL &&
        cur_right->type == cur_left->type) {
        return 1;
    }
    return 0;
}

struct Type *pointer_arithmatic_res_type(struct Type *left, struct Type *right,
                                         int *left_needs_cast,
                                         int *right_needs_cast) {
    if (left->type == T_POINTER && right->type == T_POINTER) {
        if (types_are_eq(left, right)) {
            PTR_RETURN(left_needs_cast, 0)
            PTR_RETURN(right_needs_cast, 0)
            return &TTULONG;
        }
        return NULL;
    }

    struct Type *ptr;
    struct Type *must_be_int;
    int *ptr_needs_cast;
    int *int_needs_cast;

    if (left->type == T_POINTER) {
        ptr = left;
        must_be_int = right;
        ptr_needs_cast = left_needs_cast;
        int_needs_cast = right_needs_cast;
    } else {
        ptr = right;
        must_be_int = left;
        ptr_needs_cast = right_needs_cast;
        int_needs_cast = left_needs_cast;
    }

    // make sure must_be_int is a ptr
    enum Types itype = get_last_from_next(must_be_int)->type;
    if (itype != T_INT && itype != T_SHORT && itype != T_CHAR) {
        return NULL;
    }

    PTR_RETURN(int_needs_cast, must_be_int->type != T_INT);
    PTR_RETURN(ptr_needs_cast, 0);
    return ptr;
}

struct Type *arithmatic_res_type(struct Type *left, struct Type *right,
                                 int allow_pointer, int *left_needs_cast,
                                 int *right_needs_cast) {
    if (left->type == T_POINTER || right->type == T_POINTER) {
        if (!allow_pointer) {
            yyerror("Pointer type can'd do this arithmetic op");
            exit(2);
        }
        return pointer_arithmatic_res_type(left, right, left_needs_cast,
                                           right_needs_cast);
    }
    if (!type_can_do_math(left) || !type_can_do_math(right)) {
        return NULL;
    }
    // if they aren't they same base type
    enum Types l_left;
    enum Types l_right;
    if ((l_left = get_last_from_next(left)->type) !=
        (l_right = get_last_from_next(right)->type)) {
        if (l_left - l_right > 0) {
            PTR_RETURN(left_needs_cast, 0)
            PTR_RETURN(right_needs_cast, 1)
            return left;
        }
        PTR_RETURN(left_needs_cast, 1)
        PTR_RETURN(right_needs_cast, 0)
        return right;
    }

    // if they are the same base type
    int left_has_unsigned;
    int right_has_unsigned;
    int left_len = get_long_len(left, &left_has_unsigned) - left_has_unsigned;
    int right_len =
        get_long_len(right, &right_has_unsigned) - right_has_unsigned;

    if (left_len < right_len) {
        PTR_RETURN(left_needs_cast, 1)
        PTR_RETURN(right_needs_cast, 0)
        return right;
    }
    if (left_len > right_len) {
        PTR_RETURN(left_needs_cast, 0)
        PTR_RETURN(right_needs_cast, 1)
        return left;
    }
    if (left_has_unsigned < right_has_unsigned) {
        PTR_RETURN(left_needs_cast, 1)
        PTR_RETURN(right_needs_cast, 0)
        return right;
    }
    if (left_has_unsigned > right_has_unsigned) {
        PTR_RETURN(left_needs_cast, 0)
        PTR_RETURN(right_needs_cast, 1)
        return left;
    }
    PTR_RETURN(left_needs_cast, 0)
    PTR_RETURN(right_needs_cast, 0)
    return left;
}

AstNode *make_binary_op(int op, AstNode *left, AstNode *right) {
    AstNode *ast = make_AstNode(ASTNODE_BINARYOP);
    struct BinaryOp *bo = &ast->binary_op;
    bo->op = op;
    bo->left = left;
    bo->right = right;
    promote_arr(bo->left);
    promote_arr(bo->right);

    // math binary ops
    if (op == '+' || op == '-' || op == '*' || op == '/' || op == PLUSEQ ||
        op == MINUSEQ || op == TIMESEQ || op == DIVEQ) {
        int left_cast;
        int right_cast;
        if ((ast->value_type = arithmatic_res_type(
                 left->value_type, right->value_type,
                 op == '+' || op == '-' || op == PLUSEQ || op == MINUSEQ,
                 &left_cast, &right_cast)) == NULL) {
            yyerror("Invalid arugment for math expression");
            exit(2);
        };
        // add the cast in there
        if (left_cast) {
            bo->left =
                make_CastStatment(left, make_Typename(right->value_type));
        }
        if (right_cast) {
            bo->right =
                make_CastStatment(right, make_Typename(left->value_type));
        }

        return ast;
    }
    // Integer only
    if (op == '%' || op == EQEQ || op == NOTEQ || op == '<' || op == LTEQ ||
        op == '>' || op == GTEQ || op == LOGAND || op == LOGOR || op == MODEQ ||
        op == SHLEQ || op == SHREQ || op == ANDEQ || op == XOREQ ||
        op == OREQ || op == '&' || op == '|' || op == '^' || op == SHL ||
        op == SHR) {
        if (get_last_from_next(left->value_type)->type != T_INT ||
            get_last_from_next(right->value_type)->type != T_INT) {
            yyerror("Invalid Type for Integer only expression");
            exit(2);
        }
        int left_cast;
        int right_cast;
        ast->value_type = arithmatic_res_type(
            left->value_type, right->value_type, 0, &left_cast, &right_cast);
        // add the cast in there
        if (left_cast) {
            bo->left =
                make_CastStatment(left, make_Typename(right->value_type));
        }
        if (right_cast) {
            bo->right =
                make_CastStatment(right, make_Typename(left->value_type));
        }
        return ast;
    }

    if (op == '.' || op == INDSEL) {
        struct Type *left_type = left->value_type;
        if (op == INDSEL) {
            if (left_type->type != T_POINTER) {
                yyerror("Tried to -> a non pointer type");
                exit(2);
            }
            left_type = left_type->extentions.next_type.next;
        }
        // check normal struct stuff
        if (left_type->type != T_STRUCT && left_type->type != T_UNION) {
            yyerror("Tried to select a non struct or union");
            exit(2);
        }
        ast->value_type = right->value_type;
        return ast;
    }
    if (op == ',')
        return ast;

    if (op == '=') {
        if (types_are_eq(left->value_type, right->value_type)) {
            ast->value_type = left->value_type;
            return ast;
        }
        if (type_can_do_math(left->value_type) &&
            type_can_do_math(right->value_type)) {
            int left_cast, right_cast;
            struct Type *res =
                arithmatic_res_type(left->value_type, right->value_type, 1,
                                    &left_cast, &right_cast);
            if (res == NULL || left_cast) {
                yyerror("Invalid assignment operator");
                exit(2);
            }
            if (right_cast) {
                bo->right =
                    make_CastStatment(right, make_Typename(left->value_type));
            }
            ast->value_type = left->value_type;
            return ast;
        }
    }
    fprintf(stderr, "Unsuportted op = %d", op);
    exit(1);
}

AstNode *make_unary_op(int op, AstNode *child) {
    AstNode *ast = make_AstNode(ASTNODE_UNARYOP);
    struct UnaryOp *uo = &ast->unary_op;
    uo->op = op;
    uo->child = child;

    switch (op) {
    case '*': {
        if (child->value_type->type != T_POINTER) {
            yyerror("Invalid Argument to Dref Operator");
            exit(2);
        }
        ast->value_type = child->value_type->extentions.next_type.next;
        promote_arr(ast);
        return ast;
    }
    case '&': {
        // this will never be freed should likely make an allocator or something
        // if I was concerned with that. But we'll leak for now
        struct Type *ptr = make_next_type(T_POINTER, child->value_type);
        ast->value_type = ptr;
        return ast;
    } break;
    case '+':
        if (!type_can_do_math(child->value_type)) {
            yyerror("Invalid Argument to +");
            exit(2);
        }
        break;
    case '-': {
        if (!type_can_do_math(child->value_type)) {
            yyerror("Invalid Argument to -");
            exit(2);
        }
    } break;
    case '~':
        if (!(get_last_from_next(child->value_type)->type == T_INT)) {
            yyerror("Invalid Argument to ~");
            exit(2);
        }
        break;
    case '!':
        if (!(get_last_from_next(child->value_type)->type == T_INT)) {
            yyerror("Invalid Argument to !");
            exit(2);
        }
        break;
    case PLUSEQ:
        if (!type_can_do_math(child->value_type)) {
            yyerror("Invalid Argument to +=");
            exit(2);
        }
        break;
    case MINUSEQ:
        if (!type_can_do_math(child->value_type)) {
            yyerror("Invalid Argument to -=");
            exit(2);
        }
        break;
    case PLUSPLUS:
        if (!(get_last_from_next(child->value_type)->type == T_INT)) {
            yyerror("Invalid Argument to ++");
            exit(2);
        }
        break;
    case MINUSMINUS:
        if (!(get_last_from_next(child->value_type)->type == T_INT)) {
            yyerror("Invalid Argument to --");
            exit(2);
        }
        break;
    case SIZEOF:
        // let everything through for now
        // don't promote array
        ast->value_type = child->value_type;
        return ast;
        break;
    default:
        fprintf(stderr, "UNREACHABLE\n");
        exit(1);
    }
    ast->value_type = child->value_type;
    promote_arr(ast);
    return ast;
}

AstNode *make_IdentNode(YYlvalStrLit val) {
    AstNode *ast = make_AstNode(ASTNODE_IDENT);
    if (!find_in_namespace(val.str, ANY, &ast->ident)) {
        fprintf(stderr, "Ident: %s\n", val.str);
        yyerror("Ident referenced before assignment");
        exit(2);
    };
    ast->value_type = ast->ident->val.type;
    return ast;
}

AstNode *make_ternary_op(AstNode *cond, AstNode *truthy, AstNode *falsey) {
    AstNode *ast = make_AstNode(ASTNODE_TERNAYROP);
    ast->ternary_op.cond = cond;
    ast->ternary_op.truthy = truthy;
    ast->ternary_op.falsey = falsey;
    if (!(cond->value_type->type == T_INT ||
          (cond->value_type->type == T_LONG &&
           get_last_from_next(cond->value_type)->type == T_INT))) {
        yyerror("Invalid Argument");
        exit(2);
    }

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

AstNode *make_CastStatment(AstNode *val, AstNode *type_name) {
    AstNode *ast = make_AstNode(ASTNODE_CAST);
    ast->cast_statment.val = val;
    ast->cast_statment.type_name = type_name;
    ast->value_type = type_name->value_type;
    return ast;
}
AstNode *make_Typename(struct Type *type_name) {
    AstNode *ast = make_AstNode(ASTNODE_TYPENAME);
    ast->value_type = type_name;
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
        printf("Constant of type ");
        print_type(head->value_type);
        printf(" = ");
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
            "Decleration: %s namespace (%d) ident_type (%d) sc (%d) sl(%d) "
            "type: ",
            head->declaration.symbol->name, head->declaration.symbol->namespc,
            head->declaration.symbol->type, head->declaration.symbol->val.sc.sd,
            head->declaration.symbol->val.sc.sl);
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
    case ASTNODE_CAST:
        printf("Cast Statment: \n");
        print_AstNode(head->cast_statment.val, tab_count + 1);
        add_tab(tab_count);
        printf("as: \n");
        add_tab(tab_count + 1);
        print_type(head->cast_statment.type_name->value_type);
        return;
    case ASTNODE_TYPENAME:
        printf("Type :\n");
        add_tab(tab_count + 1);
        print_type(head->value_type);
        return;

    default:
        fprintf(stderr, "Unsuportted Node type %d\n", head->type);
        exit(1);
    }
}
