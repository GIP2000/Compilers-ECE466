#include "./ast_parser.h"
#include "../parser/types.h"
#include "./quad.h"
#include <stdio.h>

void parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Quad *pass);

struct BasicBlockArr build_bba_from_st(struct SymbolTable *st) {
    struct BasicBlockArr bba = initalize_BasicBlockArr(100);

    size_t i;
    for (i = 0; i < st->len; ++i) {
        if (st->nodearr[i]->type != FUNCTION ||
            st->nodearr[i]->val.type->extentions.func.statment != NULL)
            continue;
        append_basic_block(&bba, make_bb(st->nodearr[i]));
        parse_ast(&bba, st->nodearr[i]->val.type->extentions.func.statment,
                  NULL);
    }

    return bba;
}

void parse_ast(struct BasicBlockArr *bba, AstNode *ast, struct Quad *pass) {
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
        break;
    case ASTNODE_BINARYOP:
        break;
    case ASTNODE_TERNAYROP:
        break;
    case ASTNODE_FUNCCALL:
        break;
    case ASTNODE_STATMENTLIST: {
        struct StatmentListNode *stln;
        for (stln = ast->statments.head; stln != NULL; stln = stln->next) {
            parse_ast(bba, stln->node, pass);
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
