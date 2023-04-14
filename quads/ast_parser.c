#include "./ast_parser.h"
#include "../macro_util.h"
#include "../parser.tab.h"
#include "./quad.h"
#include <stdio.h>

extern SIZEOF_TABLE TYPE_SIZE_TABLE;
extern VReg next_vreg;
typedef unsigned long long u64;
int parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Location *pass);
u64 size_of_abstract(struct Type *t);
u64 get_struct_size(struct Type *t);
u64 get_union_size(struct Type *t);

u64 get_struct_or_union_size(struct Type *t) {
    if (t->extentions.st_un.is_cached)
        return t->extentions.st_un.cached_size;
    if (t->extentions.st_un.is_struct)
        return get_struct_size(t);
    return get_union_size(t);
}

u64 get_union_size(struct Type *t) {
    struct SymbolTable *mems = t->extentions.st_un.mem;
    size_t i;
    u64 max = 0;
    for (i = 0; i < mems->len; ++i) {
        u64 current = size_of_abstract(mems->nodearr[i]->val.type);
        max = current > max ? current : max;
    }
    return max + (max % 4);
}

u64 get_struct_size(struct Type *t) {

    // if its not cached them
    // generate and cache
    struct SymbolTable *mems = t->extentions.st_un.mem;
    size_t i;
    u64 alignment = 0;
    u64 total = 0;
    for (i = 0; i < mems->len; ++i) {
        u64 current = size_of_abstract(mems->nodearr[i]->val.type);
        alignment += current;
        if (alignment > 4) {
            total += alignment % 4;
            alignment = 0;
        }
        mems->nodearr[i]->offset_marked = 1;
        mems->nodearr[i]->offset = total;
        total += current;
    }
    t->extentions.st_un.cached_size = total;
    t->extentions.st_un.is_cached = 1;
    return total;
}

u64 get_array_length(struct Type *t) {

    AstNode *b_expr = t->extentions.next_type.arr_size_expression;
    if (b_expr->type != ASTNODE_CONSTANT) {
        fprintf(stderr, "UNIMPLEMENTED non conststant value in array []");
        exit(1);
    }
    ConstantTypes ct = b_expr->constant.type;
    if (ct > TULONGLONG) {
        fprintf(stderr, "Invalid arugment to array, must be Integer");
        exit(3);
    }
    if (ct < TUINT && (long long)b_expr->constant.val.u_int < 0) {
        fprintf(stderr, "Invalid argument to array, must be positive number");
        exit(3);
    }
    return b_expr->constant.val.u_int;
}

u64 size_of_abstract(struct Type *t) {
    long long result = TYPE_SIZE_TABLE[t->type];
    if (result > 0) {
        return result;
    }
    if (result == 0) {
        fprintf(stderr, "Invalid argument to sizeof");
        exit(3);
    }
    // is struct, union or array
    if (t->type == T_ARR) {
        return get_array_length(t) *
               size_of_abstract(t->extentions.next_type.next);
    }
    return get_struct_or_union_size(t);
}

struct BasicBlockArr build_bba_from_st(struct SymbolTable *st) {
    struct BasicBlockArr bba = initalize_BasicBlockArr(100);

    size_t i;
    for (i = 0; i < st->len; ++i) {
        if (st->nodearr[i]->type != FUNCTION ||
            st->nodearr[i]->val.type->extentions.func.statment == NULL) {
            fprintf(stderr, "skipping non funciton symbol %s with type %d\n",
                    st->nodearr[i]->name, st->nodearr[i]->type);
            continue;
        }
        append_basic_block(&bba, make_bb(st->nodearr[i]));
        // TODO parse for all declarators
        parse_ast(&bba, st->nodearr[i]->val.type->extentions.func.statment,
                  NULL);
    }

    return bba;
}

int is_lvalue(AstNode *node) {
    if (node->type == ASTNODE_IDENT ||
        (node->type == ASTNODE_UNARYOP && node->unary_op.op == '*'))
        return 1;
    return 0;
}

struct Location get_loc_from_parse_ast(int is_val, AstNode *node,
                                       struct BasicBlockArr *bba,
                                       struct Quad *out_quad) {
    struct Location result;
    if (is_val) {
        if (node->type == ASTNODE_CONSTANT) {
            if (node->constant.type >= TDOUBLE &&
                node->constant.type <= TLONGDOUBLE)
                result = make_Location_float(node->constant.val.flt);
            else
                result = make_Location_int(node->constant.val.u_int);

        } else
            result = make_Location_var(node->ident);
    } else
        result = bba->arr[bba->len - 1].tail->quad.eq;

    if (out_quad != NULL && bba->arr[bba->len - 1].tail != NULL) {
        *out_quad = bba->arr[bba->len - 1].tail->quad;
    }

    return result;
}

void parse_unary_op(struct BasicBlockArr *bba, struct UnaryOp *uop,
                    struct Location *eq) {

    struct Location eq_r;
    if (eq == NULL) {
        eq_r = make_Location_reg();
    } else {
        eq_r = *eq;
    }
    struct Location arg2 = make_Location_empty_reg();

    switch (uop->op) {
    case '&': {
        if (!is_lvalue(uop->child)) {
            fprintf(stderr, "Can't take the address of an rvalue\n");
            exit(3);
        }
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, LEA, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '*': {
        if (uop->child->value_type->type != T_POINTER) {
            fprintf(stderr, "Can't Deref a non pointer type");
        }
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, LOAD, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '+':
        // IDK what this is supposed to do
        // I think this is just a noop
        break;
    case '-': {
        enum Operation op;
        struct Type *last = get_last_from_next(uop->child->value_type);
        if (last->type == T_FLOAT || last->type == T_DOUBLE) {
            op = FMUL;
        } else {
            op = MUL;
        }
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, op, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '~': {
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, BINOT, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '!': {
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, LOGNOT, arg1, arg2);

        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    case PLUSPLUS: {
        if (eq != NULL) {
            fprintf(stderr, "Warning: This shouldn't happen, something fishy "
                            "is going on (PLUSPLUS has eq)");
        }
        struct Type *last = get_last_from_next(uop->child->value_type);
        if (last->type != T_INT && last->type != T_SHORT &&
            last->type != T_CHAR && last->type != T_LONG &&
            !is_lvalue(uop->child)) {
            fprintf(stderr, "Invalid ++ expression\n");
            exit(3);
        }
        int is_val = parse_ast(bba, uop->child, NULL);

        // this isn't efficent since im doing some moving around
        // but lets pretend im going to have an optimizer that
        // will fix this
        fprintf(stderr, "HERE 1\n");
        struct Quad last_quad;
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, &last_quad);
        fprintf(stderr, "HERE 2\n");
        print_quad(&last_quad);
        printf("\n");
        struct Location one_const = make_Location_int(1);
        // cache
        struct Location orig_cache = make_Location_reg();
        struct Quad cache_q = make_quad(orig_cache, MOV, arg1, arg2);
        append_quad(&bba->arr[bba->len - 1], cache_q);
        // do the add
        struct Location origin;
        if (uop->child->type == ASTNODE_IDENT) {
            origin = make_Location_var(uop->child->ident);
        } else {
            origin = last_quad.arg1;
            origin.deref = 1;
        }
        struct Quad q = make_quad(origin, ADD, arg1, one_const);
        append_quad(&bba->arr[bba->len - 1], q);
        // }
        // move the new value this is dumb but whatever
        struct Location new_cache = make_Location_reg();
        struct Quad result = make_quad(new_cache, MOV, orig_cache, arg2);
        return append_quad(&bba->arr[bba->len - 1], result);
    }
    case MINUSMINUS: {
        if (eq != NULL) {
            fprintf(stderr, "Warning: This shouldn't happen, something fishy "
                            "is going on (MINUSMINUS has eq)");
        }
        struct Type *last = get_last_from_next(uop->child->value_type);
        if (last->type != T_INT && last->type != T_SHORT &&
            last->type != T_CHAR && last->type != T_LONG &&
            !is_lvalue(uop->child)) {
            fprintf(stderr, "Invalid ++ expression\n");
            exit(3);
        }
        int is_val = parse_ast(bba, uop->child, NULL);

        // this isn't efficent since im doing some moving around
        // but lets pretend im going to have an optimizer that
        // will fix this
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Location one_const = make_Location_int(1);

        // cache
        struct Location orig_cache = make_Location_reg();
        struct Quad cache_q = make_quad(orig_cache, MOV, arg1, arg2);
        append_quad(&bba->arr[bba->len - 1], cache_q);
        // do the add
        struct Location origin = make_Location_var(uop->child->ident);
        if (uop->child->type != ASTNODE_IDENT) {
            origin.deref = 1;
        }
        struct Quad q = make_quad(origin, SUB, arg1, one_const);
        append_quad(&bba->arr[bba->len - 1], q);
        // move the new value
        struct Location new_cache = make_Location_reg();
        struct Quad result = make_quad(new_cache, MOV, orig_cache, arg2);
        return append_quad(&bba->arr[bba->len - 1], result);
    }
    case SIZEOF: {
        if (uop->child->value_type == NULL) {
            fprintf(stderr, "Invalid sizeof expression\n");
            exit(3);
        }
        long long size = size_of_abstract(uop->child->value_type);
        // Don't generate code inside of sizeof
        struct Location arg1 = make_Location_int(size);
        struct Quad q = make_quad(eq_r, MOV, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    default:
        fprintf(stderr, "Invalid Unary operator %c", (char)uop->op);
        exit(3);
    }
}

void parse_binary_op(struct BasicBlockArr *bba, struct BinaryOp *bop,
                     struct Location *eq) {
    struct Location eq_r;
    if (eq == NULL) {
        eq_r = make_Location_reg();
    } else {
        eq_r = *eq;
    }
    switch (bop->op) {
    // deafult math stuff
    case '+': {
        if (bop->left->value_type->type == T_POINTER &&
            bop->right->value_type->type == T_POINTER) {
            fprintf(stderr, "Can't add two pointers together");
            exit(3);
        }
        if (bop->left->value_type->type == T_POINTER ||
            bop->right->value_type->type == T_POINTER) {
            // check to make sure one of them is an integer type also
            // do pointer arithmatic stuff
            return;
        }
        int is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, bop->left, bba, NULL);
        is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg2 =
            get_loc_from_parse_ast(is_val, bop->right, bba, NULL);
        enum Types op_type = get_last_from_next(bop->right->value_type)->type;
        enum Operation op =
            op_type == T_FLOAT || op_type == T_DOUBLE ? FADD : ADD;
        struct Quad q = make_quad(eq_r, op, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '-':
        break;
    case '*':
        break;
    case '/':
        break;
    case '%':
        break;
    case PLUSEQ:
        break;
    case MINUSEQ:
        break;
    case TIMESEQ:
        break;
    case DIVEQ:
        break;
    }
}

int parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Location *eq) {
    switch (ast->type) {
    case ASTNODE_CONSTANT:
        return 1;
    case ASTNODE_STRLIT:
        break;
    case ASTNODE_IDENT:
        return 1;
    case ASTNODE_UNARYOP:
        parse_unary_op(bba, &ast->unary_op, eq);
        break;
    case ASTNODE_BINARYOP:
        parse_binary_op(bba, &ast->binary_op, eq);
        break;
    case ASTNODE_TERNAYROP:
        break;
    case ASTNODE_FUNCCALL:
        break;
    case ASTNODE_STATMENTLIST: {
        struct StatmentListNode *stln;
        for (stln = ast->statments.head; stln != NULL; stln = stln->next) {
            parse_ast(bba, stln->node, eq);
        }
        break;
    }
    case ASTNODE_DECLARATION:
        // this is prob a noop
        // I need to declare all locals at the beg of a fucntion
        break;
    case ASTNODE_IF_STATMENT:
        break;
    case ASTNODE_FOR_STATMENT:
        break;
    case ASTNODE_WHILE_STATMENT:
        break;
    case ASTNODE_GOTO_STATMENT:
        break;
    case ASTNODE_CONTINUE_STATMENT:
        break;
    case ASTNODE_BREAK_STATMENT:
        break;
    case ASTNODE_RETURN_STATMENT:
        break;
    case ASTNODE_LABEL_STATMENT:
        break;
    case ASTNODE_SWITCH_STATMENT:
        break;
    case ASTNODE_CASE_STATMENT:
        break;
    case ASTNODE_DEFAULT_STATMENT:
        break;
    case ASTNODE_CAST:
        // should only have to do something
        // if I am going from floating point to integer
        // otherwise noop
        break;
    }
    return 0;
}
