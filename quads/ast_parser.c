#include "./ast_parser.h"
#include "../parser.tab.h"
#include "./quad.h"
#include <stdio.h>

extern SIZEOF_TABLE TYPE_SIZE_TABLE;
extern VReg next_vreg;
typedef unsigned long long u64;
void parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Location *pass);
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

void parse_unary_op(struct BasicBlockArr *bba, struct UnaryOp *uop,
                    struct Location *eq) {

    struct Location eq_r;
    if (eq == NULL) {
        eq_r.reg = next_vreg++;
    } else {
        eq_r = *eq;
    }

    switch (uop->op) {
    case '&':
        if (!is_lvalue(uop->child)) {
            fprintf(stderr, "Can't take the address of an rvalue\n");
            exit(3);
        }
        parse_ast(bba, uop->child, NULL);
        struct Location arg1 = bba->arr[bba->len - 1].tail->quad.eq;
        struct Location arg2;
        struct Quad q = make_quad(eq_r, LEA, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    case '*': {
        // TODO check the type of uop->child to see if its a pointer type
        struct Location arg2;
        parse_ast(bba, uop->child, NULL);
        struct Location arg1 = bba->arr[bba->len - 1].tail->quad.eq;
        struct Quad q = make_quad(eq_r, LOAD, arg1, arg2);
        return append_quad(&bba->arr[bba->len - 1], q);
    }
    case '+':
        break;
    case '-':
        break;
    case '~':
        break;
    case '!':
        break;
    case PLUSEQ:
        break;
    case MINUSEQ:
        break;
    case PLUSPLUS:
        break;
    case MINUSMINUS:
        break;
    case SIZEOF:
        break;
    default:
        fprintf(stderr, "Invalid Unary operator %c", (char)uop->op);
        exit(3);
    }
}

void parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Location *eq) {
    struct BasicBlock *bb = &bba->arr[bba->len - 1];
    switch (ast->type) {
    case ASTNODE_CONSTANT:
        fprintf(stderr, "Warning: useless constant");
        break;
    case ASTNODE_STRLIT:
        fprintf(stderr, "Warning: useless Str lit");
        break;
    case ASTNODE_IDENT:
        fprintf(stderr, "Warning: useless Str lit");
        break;
    case ASTNODE_UNARYOP:
        return parse_unary_op(bba, &ast->unary_op, eq);
    case ASTNODE_BINARYOP:
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
    }
}
