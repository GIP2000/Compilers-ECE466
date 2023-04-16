#include "./ast_parser.h"
#include "../macro_util.h"
#include "../parser.tab.h"
#include "./quad.h"
#include <stdio.h>

extern SIZEOF_TABLE TYPE_SIZE_TABLE;
const OpInverter INVERTER = {1, 0, 5, 3, 4, 2};
extern VReg next_vreg;
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
    if (t->extentions.st_un.is_cached) {
        return t->extentions.st_un.cached_size;
    }
    // generate and cache
    struct SymbolTable *mems = t->extentions.st_un.mem;
    size_t i;
    u64 alignment = 0;
    u64 total = 0;
    for (i = 0; i < mems->len; ++i) {
        u64 current = size_of_abstract(mems->nodearr[i]->val.type);

        alignment += current;
        if (alignment >= 4) {
            if (alignment != 4)
                total += 4 - (alignment - current);
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
    if (b_expr == NULL) {
        fprintf(stderr, "Can't get size of incomplete type arr empty\n");
        exit(3);
    }

    if (b_expr->type != ASTNODE_CONSTANT) {
        fprintf(stderr, "UNIMPLEMENTED non conststant value in array []\n");
        exit(1);
    }
    ConstantTypes ct = b_expr->constant.type;
    if (ct > TULONGLONG) {
        fprintf(stderr, "Invalid arugment to array, must be Integer\n");
        exit(3);
    }
    if (ct < TUINT && (i64)b_expr->constant.val.u_int < 0) {
        fprintf(stderr, "Invalid argument to array, must be positive number\n");
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
        (node->type == ASTNODE_UNARYOP && node->unary_op.op == '*') ||
        (node->type == ASTNODE_BINARYOP &&
         (node->binary_op.op == '.' || node->binary_op.op == INDSEL)))
        return 1;
    return 0;
}

struct Location get_loc_pos_is_val(AstNode *node) {

    struct Location result;
    if (node->type == ASTNODE_CONSTANT) {
        if (node->constant.type >= TDOUBLE &&
            node->constant.type <= TLONGDOUBLE) {
            result = make_Location_float(node->constant.val.flt);
        } else
            result = make_Location_int(node->constant.val.u_int);

    } else {
        result = make_Location_var(node->ident);
    }
    return result;
}

struct Location get_loc_from_parse_ast(int is_val, AstNode *node,
                                       struct BasicBlockArr *bba,
                                       struct Quad *out_quad) {
    if (node->type == ASTNODE_CAST) {
        return get_loc_from_parse_ast(is_val, node->cast_statment.val, bba,
                                      out_quad);
    }
    struct Location result;
    if (is_val) {
        result = get_loc_pos_is_val(node);
    } else {
        int i;
        for (i = (int)bba->len - 1; i >= 0; --i) {
            if (bba->arr[i].tail != NULL) {
                result = bba->arr[i].tail->quad.eq;
                // TODO make sable happy
                goto outside;
            }
        }
        fprintf(stderr, "Something is very fucked\n");
        exit(1);
    }

outside:

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
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    case '*': {
        if (uop->child->value_type->type != T_POINTER) {
            fprintf(stderr, "Can't Deref a non pointer type");
        }
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, LOAD, arg1, arg2);
        append_quad(&bba->arr[bba->len - 1], q);
        return;
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
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    case '~': {
        int is_val = parse_ast(bba, uop->child, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, NULL);
        struct Quad q = make_quad(eq_r, BINOT, arg1, arg2);
        append_quad(&bba->arr[bba->len - 1], q);
        return;
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
        struct Quad last_quad;
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, uop->child, bba, &last_quad);
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
        append_quad(&bba->arr[bba->len - 1], result);
        return;
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
        append_quad(&bba->arr[bba->len - 1], result);
        return;
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
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    default:
        fprintf(stderr, "Invalid Unary operator %c", (char)uop->op);
        exit(3);
    }
}

void do_assignment_math(struct BasicBlockArr *bba, struct BinaryOp *bop,
                        enum Operation op_g) {
    int is_val = parse_ast(bba, bop->left, NULL);
    struct Quad last_quad;
    struct Location arg1_val =
        get_loc_from_parse_ast(is_val, bop->left, bba, &last_quad);
    struct Type *op_type = get_last_from_next(bop->right->value_type);
    enum Operation op =
        op_g + (op_type->type == T_FLOAT || op_type->type == T_DOUBLE) * 4;

    if (bop->left->type != ASTNODE_IDENT) {
        int is_val_2 = parse_ast(bba, bop->right, NULL);
        struct Location arg2 =
            get_loc_from_parse_ast(is_val_2, bop->right, bba, NULL);
        struct Location arg1 = arg1_val;
        if (!is_val) {
            arg1 = last_quad.arg1;
            arg1.deref = 1;
        }
        struct Quad q = make_quad(arg1, op, arg2, arg1_val);
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    is_val = parse_ast(bba, bop->right, NULL);
    struct Location arg2 =
        get_loc_from_parse_ast(is_val, bop->right, bba, NULL);

    struct Quad q = make_quad(arg1_val, op, arg1_val, arg2);
    append_quad(&bba->arr[bba->len - 1], q);
    return;
}

void do_normal_math(struct BasicBlockArr *bba, struct BinaryOp *bop,
                    struct Location *eq_r, enum Operation op_g) {
    int is_val = parse_ast(bba, bop->left, NULL);
    struct Location arg1 = get_loc_from_parse_ast(is_val, bop->left, bba, NULL);
    is_val = parse_ast(bba, bop->right, NULL);
    struct Location arg2 =
        get_loc_from_parse_ast(is_val, bop->right, bba, NULL);
    enum Types op_type = get_last_from_next(bop->right->value_type)->type;
    int is_float = op_type == T_FLOAT || op_type == T_DOUBLE;
    enum Operation op = op_g + (is_float * 4);
    struct Quad q = make_quad(*eq_r, op, arg1, arg2);
    append_quad(&bba->arr[bba->len - 1], q);
    return;
}

enum Operation invert_cmp(enum Operation op) {
    if (op < BREQ) {
        fprintf(stderr, "UNRECHABLE\n");
        exit(1);
    }

    enum Operation start;
    if (op <= BRGE)
        start = BREQ;
    else if (op <= BRGEU)
        start = BREQU;
    else if (op <= CCGE)
        start = CCEQ;
    else if (op <= CCGEU)
        start = CCEQU;
    else {
        fprintf(stderr, "UNRECHABLE\n");
        exit(1);
    }

    return INVERTER[op - start] + start;
}

enum Operation get_op_from_child(struct BasicBlockArr *bba, AstNode *child) {
    int is_val = parse_ast(bba, child, NULL);
    struct Quad last_q;
    struct Location arg1 = get_loc_from_parse_ast(is_val, child, bba, &last_q);
    if (bba->arr[bba->len - 1].tail == NULL && bba->len > 1) {
        // if this is nested then grab the last thing
        last_q = bba->arr[bba->len - 2].tail->quad;
        if (!is_val) {
            arg1 = bba->arr[bba->len - 2].tail->quad.eq;
        }
    }
    if (!is_val && last_q.op >= BREQ && last_q.op <= CCGEU) {
        return last_q.op;
    }

    struct Location cmp_val =
        make_Location_int(child->value_type->type != T_POINTER);
    struct Quad q = make_quad(make_Location_empty_reg(), CMP, arg1, cmp_val);
    append_quad(&bba->arr[bba->len - 1], q);
    return CCEQ + (CMPLEN * (child->value_type->type == T_UNSIGNED));
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

            AstNode *ptr;
            AstNode *integer;

            if (bop->left->value_type->type == T_POINTER) {
                ptr = bop->left;
                integer = bop->right;
            } else {
                ptr = bop->left;
                integer = bop->right;
            }

            u64 size =
                size_of_abstract(ptr->value_type->extentions.next_type.next);
            // mul size by integer
            int is_val = parse_ast(bba, integer, NULL);
            struct Location arg1 =
                get_loc_from_parse_ast(is_val, integer, bba, NULL);
            struct Location arg2 = make_Location_int((i64)size);
            struct Location mul_result = make_Location_reg();
            struct Quad mul_q = make_quad(mul_result, MUL, arg1, arg2);
            append_quad(&bba->arr[bba->len - 1], mul_q);
            is_val = parse_ast(bba, ptr, NULL);
            arg1 = get_loc_from_parse_ast(is_val, ptr, bba, NULL);
            struct Quad q = make_quad(eq_r, ADD, arg1, mul_result);

            append_quad(&bba->arr[bba->len - 1], q);
            return;
        }
        return do_normal_math(bba, bop, &eq_r, ADD);
    }
    case '-': {
        if (bop->left->value_type->type == T_POINTER ^
            bop->right->value_type->type == T_POINTER) {
            // do pointer arithmatic stuff

            AstNode *ptr;
            AstNode *integer;

            if (bop->left->value_type->type == T_POINTER) {
                ptr = bop->left;
                integer = bop->right;
            } else {
                ptr = bop->left;
                integer = bop->right;
            }

            u64 size =
                size_of_abstract(ptr->value_type->extentions.next_type.next);
            // mul size by integer
            int is_val = parse_ast(bba, integer, NULL);
            struct Location arg1 =
                get_loc_from_parse_ast(is_val, integer, bba, NULL);
            struct Location arg2 = make_Location_int((i64)size);
            struct Location mul_result = make_Location_reg();
            struct Quad mul_q = make_quad(mul_result, MUL, arg1, arg2);
            append_quad(&bba->arr[bba->len - 1], mul_q);
            is_val = parse_ast(bba, ptr, NULL);
            arg1 = get_loc_from_parse_ast(is_val, ptr, bba, NULL);
            struct Quad q = make_quad(eq_r, SUB, arg1, mul_result);

            append_quad(&bba->arr[bba->len - 1], q);
            return;
        }
        // do normal math
        return do_normal_math(bba, bop, &eq_r, SUB);
    }
    case '*':
        return do_normal_math(bba, bop, &eq_r, MUL);
    case '/':
        return do_normal_math(bba, bop, &eq_r, DIV);
    case '%':
        return do_normal_math(bba, bop, &eq_r, MOD);
    case '&':
        return do_normal_math(bba, bop, &eq_r, BIAND);
    case '|':
        return do_normal_math(bba, bop, &eq_r, BIOR);
    case '^':
        return do_normal_math(bba, bop, &eq_r, BIXOR);
    case SHL:
        return do_normal_math(bba, bop, &eq_r, BISHL);
    case SHR:
        return do_normal_math(bba, bop, &eq_r, BISHR);
    case PLUSEQ:
        return do_assignment_math(bba, bop, ADD);
    case MINUSEQ:
        return do_assignment_math(bba, bop, SUB);
    case TIMESEQ:
        return do_assignment_math(bba, bop, MUL);
    case DIVEQ:
        return do_assignment_math(bba, bop, DIV);
    case MODEQ:
        return do_assignment_math(bba, bop, MOD);
    case SHLEQ:
        return do_assignment_math(bba, bop, BISHL);
    case SHREQ:
        return do_assignment_math(bba, bop, BISHR);
    case ANDEQ:
        return do_assignment_math(bba, bop, BIAND);
    case XOREQ:
        return do_assignment_math(bba, bop, BIXOR);
    case OREQ:
        return do_assignment_math(bba, bop, BIOR);
    case '=': {
        if (!is_lvalue(bop->left)) {
            fprintf(stderr, "Can't assign to an rvalue\n");
            exit(3);
        }
        int is_val = parse_ast(bba, bop->left, NULL);
        struct Quad last_quad;
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, bop->left, bba, &last_quad);
        if (bop->left->type != ASTNODE_IDENT) {
            int is_val_2 = parse_ast(bba, bop->right, NULL);
            struct Location arg2 =
                get_loc_from_parse_ast(is_val_2, bop->right, bba, NULL);
            if (!is_val) {
                arg1 = last_quad.arg1;
                arg1.deref = 1;
            }
            struct Quad q =
                make_quad(arg1, MOV, arg2, make_Location_empty_reg());
            append_quad(&bba->arr[bba->len - 1], q);
            return;
        }
        is_val = parse_ast(bba, bop->right, &arg1);
        if (is_val) {
            struct Location l =
                get_loc_from_parse_ast(is_val, bop->right, bba, NULL);
            struct Quad q = make_quad(arg1, MOV, l, make_Location_empty_reg());
            append_quad(&bba->arr[bba->len - 1], q);
            return;
        }
        return;
    }
    // selection
    case '.': {
        // make sure the struct is initalized with offsets
        size_of_abstract(bop->left->value_type);
        print_type(bop->left->value_type);
        printf("<- struct type\n");
        u64 offset = bop->right->ident->offset;
        int is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, bop->left, bba, NULL);
        // LEA
        struct Location s_addr = make_Location_reg();
        struct Quad lea_q =
            make_quad(s_addr, LEA, arg1, make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], lea_q);
        // ADD
        struct Location mem_addr = make_Location_reg();
        struct Quad add_q =
            make_quad(mem_addr, ADD, s_addr, make_Location_int(offset));
        append_quad(&bba->arr[bba->len - 1], add_q);
        // LOAD
        struct Quad q =
            make_quad(eq_r, LOAD, mem_addr, make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    case INDSEL: {
        size_of_abstract(bop->left->value_type->extentions.next_type.next);
        u64 offset = bop->right->ident->offset;
        // ADD
        int is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, bop->left, bba, NULL);
        struct Location mem_addr = make_Location_reg();
        struct Quad add_q =
            make_quad(mem_addr, ADD, arg1, make_Location_int(offset));
        append_quad(&bba->arr[bba->len - 1], add_q);
        // LOAD
        struct Quad q =
            make_quad(eq_r, LOAD, mem_addr, make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    // logical combo
    case LOGAND: {
        enum Operation op = invert_cmp(get_op_from_child(bba, bop->left));
        if (op >= BRGEU) {
            op -= CMPLEN * 2;
        }
        // now op is a branch
        struct Quad q =
            make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        op = invert_cmp(get_op_from_child(bba, bop->right));
        if (op >= BRGEU) {
            op -= CMPLEN * 2;
        }
        q = make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                      make_Location_empty_reg());
        struct Quad *first_branch = append_quad(&bba->arr[bba->len - 1], q);
        q = make_quad(eq_r, MOV, make_Location_int(1),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        q = make_quad(make_Location_empty_reg(), op,
                      make_Location_BB(bba->len + 1),
                      make_Location_empty_reg());
        struct Quad *second_branch = append_quad(&bba->arr[bba->len - 1], q);
        append_basic_block(bba, make_bb(NULL));
        first_branch->arg1.bbn = bba->len - 1;
        q = make_quad(eq_r, MOV, make_Location_int(0),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        append_basic_block(bba, make_bb(NULL));
        second_branch->arg1.bbn = bba->len - 1;
        return;
    }
    case LOGOR: {
        enum Operation op = get_op_from_child(bba, bop->left);
        if (op >= BRGEU) {
            op -= CMPLEN * 2;
        }
        // now op is a branch
        struct Quad q =
            make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        op = get_op_from_child(bba, bop->right);
        if (op >= BRGEU) {
            op -= CMPLEN * 2;
        }
        q = make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                      make_Location_empty_reg());
        struct Quad *first_branch = append_quad(&bba->arr[bba->len - 1], q);
        q = make_quad(eq_r, MOV, make_Location_int(0),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        q = make_quad(make_Location_empty_reg(), op,
                      make_Location_BB(bba->len + 1),
                      make_Location_empty_reg());
        struct Quad *second_branch = append_quad(&bba->arr[bba->len - 1], q);
        append_basic_block(bba, make_bb(NULL));
        first_branch->arg1.bbn = bba->len - 1;
        q = make_quad(eq_r, MOV, make_Location_int(1),
                      make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        append_basic_block(bba, make_bb(NULL));
        second_branch->arg1.bbn = bba->len - 1;
        return;
    }
    default: {
        // all the comparisons
        // do the cmp
        int is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg1 =
            get_loc_from_parse_ast(is_val, bop->left, bba, NULL);
        is_val = parse_ast(bba, bop->left, NULL);
        struct Location arg2 =
            get_loc_from_parse_ast(is_val, bop->right, bba, NULL);
        struct Quad cmp_q =
            make_quad(make_Location_empty_reg(), CMP, arg1, arg2);
        append_quad(&bba->arr[bba->len - 1], cmp_q);
        enum Operation op;
        switch (bop->op) {
        case EQEQ:
            op = CCEQ;
            break;
        case NOTEQ:
            op = CCNEQ;
            break;
        case '<':
            op = CCLT;
            break;
        case '>':
            op = CCGT;
            break;
        case LTEQ:
            op = CCLE;
            break;
        case GTEQ:
            op = CCGE;
            break;
        default:
            fprintf(stderr, "UNRECHABLE op\n");
            exit(1);
        }
        if (bop->left->value_type->type == T_UNSIGNED ||
            bop->right->value_type->type == T_UNSIGNED)
            op += 6;
        struct Quad q = make_quad(eq_r, op, make_Location_empty_reg(),
                                  make_Location_empty_reg());
        append_quad(&bba->arr[bba->len - 1], q);
        return;
    }
    }
}

void fall_through_if_statment(struct BasicBlockArr *bba,
                              struct IfStatment *if_s) {

    enum Operation op = invert_cmp(get_op_from_child(bba, if_s->cmp));
    if (op >= BRGEU) {
        op -= CMPLEN * 2;
    }
    struct Quad q =
        make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                  make_Location_empty_reg());
    struct Quad *first_branch = append_quad(&bba->arr[bba->len - 1], q);
    parse_ast(bba, if_s->statment, NULL);
    append_basic_block(bba, make_bb(NULL));
    first_branch->arg1.bbn = bba->len - 1;
}

void parse_while_statment(struct BasicBlockArr *bba,
                          struct WhileStatment *while_s) {
    struct Quad *branch_to_cmp_q = NULL;
    if (!while_s->is_do) {
        // if it isn't a do add a branch to the cmp
        struct Quad q =
            make_quad(make_Location_empty_reg(), BR, make_Location_BB(bba->len),
                      make_Location_empty_reg());
        branch_to_cmp_q = append_quad(&bba->arr[bba->len - 1], q);
    }
    append_basic_block(bba, make_bb(NULL));
    size_t body_bbn = bba->len - 1;
    parse_ast(bba, while_s->statment, NULL);
    append_basic_block(bba, make_bb(NULL));
    if (branch_to_cmp_q != NULL) {
        branch_to_cmp_q->arg1.bbn = bba->len - 1;
    }
    enum Operation op = get_op_from_child(bba, while_s->cmp);
    if (op >= BRGEU) {
        op -= CMPLEN * 2;
    }
    struct Quad branc_q =
        make_quad(make_Location_empty_reg(), op, make_Location_BB(body_bbn),
                  make_Location_empty_reg());
    append_quad(&bba->arr[bba->len - 1], branc_q);
    append_basic_block(bba, make_bb(NULL));
}

void parse_for_loop(struct BasicBlockArr *bba, struct ForStatment *for_s) {}

void parse_if_statment(struct BasicBlockArr *bba, struct IfStatment *if_s) {
    if (if_s->else_statment == NULL) {
        return fall_through_if_statment(bba, if_s);
    }
    enum Operation op = get_op_from_child(bba, if_s->cmp);
    if (op >= BRGEU) {
        op -= CMPLEN * 2;
    }
    struct Quad q =
        make_quad(make_Location_empty_reg(), op, make_Location_BB(bba->len),
                  make_Location_empty_reg());
    struct Quad *branch_to_if = append_quad(&bba->arr[bba->len - 1], q);
    parse_ast(bba, if_s->else_statment, NULL);
    q = make_quad(make_Location_empty_reg(), BR, make_Location_BB(bba->len),
                  make_Location_empty_reg());
    struct Quad *branch_to_end = append_quad(&bba->arr[bba->len - 1], q);
    append_basic_block(bba, make_bb(NULL));
    branch_to_if->arg1.bbn = bba->len - 1;
    parse_ast(bba, if_s->statment, NULL);
    append_basic_block(bba, make_bb(NULL));
    branch_to_end->arg1.bbn = bba->len - 1;
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
            if (stln->node == NULL) {
                continue;
            }
            parse_ast(bba, stln->node, eq);
        }
        break;
    }
    case ASTNODE_DECLARATION:
        // this is prob a noop
        // I need to declare all locals at the beg of a fucntion
        break;
    case ASTNODE_IF_STATMENT:
        parse_if_statment(bba, &ast->if_statment);
        break;
    case ASTNODE_FOR_STATMENT:
        parse_for_loop(bba, &ast->for_statment);
        break;
    case ASTNODE_WHILE_STATMENT:
        parse_while_statment(bba, &ast->while_statment);
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
        return parse_ast(bba, ast->cast_statment.val, eq);
    }
    return 0;
}
