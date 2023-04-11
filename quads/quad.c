#include "./quad.h"
#include <stdio.h>
#include <stdlib.h>

VReg next_vreg = VREG_START;
const size_t inital_cap = 100;

struct BasicBlockArr initalize_BasicBlockArr(size_t inital_cap) {
    struct BasicBlockArr bba;
    if (inital_cap == 0)
        bba.cap = inital_cap;

    bba.len = 0;
    bba.arr = (struct BasicBlock *)malloc(sizeof(struct BasicBlock) * bba.cap);
    return bba;
}

void realloc_BBA(struct BasicBlockArr *bba) {
    bba->cap *= 2;
    bba->arr = realloc(bba->arr, sizeof(struct BasicBlock) * bba->cap);
}
void append_basic_block(struct BasicBlockArr *bba, struct BasicBlock bb) {
    if (bba->len >= bba->cap) {
        realloc_BBA(bba);
    }
    bba->arr[bba->len++] = bb;
}

struct BasicBlock make_bb(struct SymbolTableNode *ref) {
    struct BasicBlock bb;
    bb.ref = ref;
    return bb;
}

struct Quad make_quad(struct Location eq, enum Operation op,
                      struct Location arg1, struct Location arg2) {
    struct Quad q;
    q.eq = eq;
    q.op = op;
    q.arg1 = arg1;
    q.arg2 = arg2;
    return q;
}

void append_quad(struct BasicBlock *bb, struct Quad quad) {
    struct QuadListNode *qn =
        (struct QuadListNode *)malloc(sizeof(struct QuadListNode));
    qn->quad = quad;
    if (bb->head == NULL) {
        bb->head = qn;
    }
    if (bb->tail != NULL) {
        bb->tail->next = qn;
    }
    bb->tail = qn;
}

void print_location(struct Location *loc) {
    if (loc->deref)
        printf("[");
    if (loc->is_var) {
        printf("%s", loc->var->name);
        return;
    }
    printf("%%T%04d", loc->reg);
    if (loc->deref)
        printf("]");
}
void print_quad(struct Quad *q) {
    printf("Quad: ");
    print_location(&q->eq);
    printf(" = %d ", q->op);
    print_location(&q->arg1);
    printf(" ");
    print_location(&q->arg2);
}
void print_bb(struct BasicBlock *bb, int add_tab) {
    size_t i;
    struct QuadListNode *n;
    for (i = 0, n = bb->head; n != NULL; n = n->next, ++i) {
        if (add_tab)
            printf("  ");
        print_quad(&n->quad);
    }
}
void print_bba(struct BasicBlockArr *bba) {
    size_t i;

    for (i = 0; i < bba->len; ++i) {
        printf("BasicBlock #%zu\n", i);
        print_bb(&bba->arr[i], 1);
    }
}
