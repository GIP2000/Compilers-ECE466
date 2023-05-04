#include "./quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VReg next_vreg = VREG_START;
const size_t INITAL_CAP = 100;

struct VRegCounter v_reg_counter;

void debug_print_vrc() {
    i64 i;
    for (i = 0; i < v_reg_counter.cap; ++i) {
        fprintf(stderr, "%%T%lld = %zu\n", i, v_reg_counter.arr[i].count);
    }
}

void initalize_counter() {
    static int first = 1;
    if (!first) {
        first = 0;
        v_reg_counter.cap = 100;
        free(v_reg_counter.arr);
    }
    v_reg_counter.cap = 100;
    v_reg_counter.arr = (struct VRegCounterNode *)malloc(
        sizeof(struct VRegCounterNode) * v_reg_counter.cap);
    memset(v_reg_counter.arr, 0, v_reg_counter.cap);
}

void increment_location(struct Location l) {
    if (!(l.loc_type == REG && l.reg != EMPTY_VREG)) {
        return;
    }
    if (v_reg_counter.cap <= l.reg) {
        size_t old_cap = v_reg_counter.cap;
        v_reg_counter.cap = l.reg * 2;
        v_reg_counter.arr = realloc(v_reg_counter.arr, v_reg_counter.cap);
        memset(v_reg_counter.arr + v_reg_counter.cap, 0,
               v_reg_counter.cap - old_cap);
    }
    ++v_reg_counter.arr[l.reg].count;
}
void increment_quad_locs(struct Quad quad) {
    increment_location(quad.eq);
    increment_location(quad.arg1);
    increment_location(quad.arg2);
}

struct Location make_Location_int(long long v) {
    struct Location l;
    l.loc_type = CONSTINT;
    l.deref = 0;
    l.const_int = v;
    return l;
}
struct Location make_Location_BB(size_t bbn) {
    struct Location l;
    l.loc_type = BASICBLOCKNUM;
    l.deref = 0;
    l.bbn = bbn;
    return l;
}
struct Location make_Location_float(long double v) {
    struct Location l;
    l.loc_type = CONSTFLOAT;
    l.deref = 0;
    l.const_float = v;
    return l;
}
struct Location make_Location_reg() {
    struct Location l;
    l.loc_type = REG;
    l.deref = 0;
    l.reg = next_vreg++;
    return l;
}
struct Location make_Location_empty_reg() {
    struct Location l;
    l.loc_type = REG;
    l.deref = 0;
    l.reg = EMPTY_VREG;
    return l;
}
struct Location make_Location_var(struct SymbolTableNode *v) {
    struct Location l;
    l.loc_type = VAR;
    l.var = v;
    l.deref = 0;
    return l;
}
struct Location make_Location_str(YYlvalStrLit strlit) {
    struct Location l;
    l.loc_type = CONSTSTR;
    l.strlit = strlit;
    l.deref = 0;
    return l;
}

struct BasicBlockArr initalize_BasicBlockArr(size_t inital_cap) {
    struct BasicBlockArr bba;
    bba.cap = inital_cap > 0 ? inital_cap : INITAL_CAP;
    bba.len = 0;
    bba.arr = (struct BasicBlock *)malloc(sizeof(struct BasicBlock) * bba.cap);
    initalize_counter();
    return bba;
}

void realloc_BBA(struct BasicBlockArr *bba) {
    bba->cap *= 2;
    bba->arr = realloc(bba->arr, sizeof(struct BasicBlock) * bba->cap);
}
void append_basic_block(struct BasicBlockArr *bba, struct BasicBlock bb) {
    if (bb.ref == NULL) {
        bb.ref = bba->arr[bba->len - 1].ref;
    }
    if (bba->len >= bba->cap) {
        realloc_BBA(bba);
    }
    bba->arr[bba->len++] = bb;
}

struct BasicBlock make_bb(struct SymbolTableNode *ref) {
    struct BasicBlock bb;
    bb.ref = ref;
    bb.head = NULL;
    bb.tail = NULL;
    bb.last_v_reg_used = next_vreg - 1;
    bb.first_v_reg_used = next_vreg - 1;
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

struct Quad *append_quad(struct BasicBlock *bb, struct Quad quad) {
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
    increment_quad_locs(quad);
    bb->last_v_reg_used = next_vreg - 1;
    return &bb->tail->quad;
}

void print_location(struct Location *loc) {
    if (loc->deref)
        printf("[");
    if (loc->loc_type == VAR)
        printf("%s", loc->var->name);
    else if (loc->loc_type == CONSTSTR)
        printf("\"%s\"", loc->strlit.original_str);
    else if (loc->loc_type == CONSTINT)
        printf("%lld", loc->const_int);
    else if (loc->loc_type == CONSTFLOAT)
        printf("%Lg", loc->const_float);
    else if (loc->loc_type == BASICBLOCKNUM) {
        printf("BB%zu", loc->bbn);
    } else if (loc->reg != EMPTY_VREG)
        printf("%%T%04d", loc->reg);
    if (loc->deref)
        printf("]");
}

void print_op(enum Operation op) {
    switch (op) {
    case LOAD:
        printf("LOAD");
        break;
    case STORE:
        printf("STORE");
        break;
    case LEA:
        printf("LEA");
        break;
    case MOV:
        printf("MOV");
        break;
    // Math Ops
    case ADD:
        printf("ADD");
        break;
    case SUB:
        printf("SUB");
        break;
    case MUL:
        printf("MUL");
        break;
    case DIV:
        printf("DIV");
        break;
    case FADD:
        printf("FADD");
        break;
    case FSUB:
        printf("FSUB");
        break;
    case FMUL:
        printf("FMUL");
        break;
    case FDIV:
        printf("FDIV");
        break;

    // BITWISE
    case BINOT:
        printf("BINOT");
        break;

    // LOGICAL
    case LOGNOT:
        printf("LOGNOT");
        break;
    case MOD:
        printf("MOD");
        break;
    case BIAND:
        printf("BIAND");
        break;
    case BIOR:
        printf("BIOR");
        break;
    case BIXOR:
        printf("BIXOR");
        break;
    case BISHL:
        printf("BISHL");
        break;
    case BISHR:
        printf("BISHR");
        break;
    case CMP:
        printf("CMP");
        break;
    case BREQ:
        printf("BREQ");
        break;
    case BREQU:
        printf("BREQU");
        break;
    case BRNEQ:
        printf("BRNEQ");
        break;
    case BRNEQU:
        printf("BRNEQU");
        break;
    case BRLT:
        printf("BRLT");
        break;
    case BRLTU:
        printf("BRLTU");
        break;
    case BRLE:
        printf("BRLE");
        break;
    case BRLEU:
        printf("BRLEU");
        break;
    case BRGT:
        printf("BRGT");
        break;
    case BRGTU:
        printf("BRGTU");
        break;
    case BRGE:
        printf("BRGE");
        break;
    case BRGEU:
        printf("BRGEU");
        break;
    case CCEQ:
        printf("CCEQ");
        break;
    case CCEQU:
        printf("CCEQU");
        break;
    case CCNEQ:
        printf("CCNEQ");
        break;
    case CCNEQU:
        printf("CCNEQU");
        break;
    case CCLT:
        printf("CCLT");
        break;
    case CCLTU:
        printf("CCLTU");
        break;
    case CCLE:
        printf("CCLE");
        break;
    case CCLEU:
        printf("CCLEU");
        break;
    case CCGT:
        printf("CCGT");
        break;
    case CCGTU:
        printf("CCGTU");
        break;
    case CCGE:
        printf("CCGE");
        break;
    case CCGEU:
        printf("CCGEU");
        break;
    case BR:
        printf("BR");
        break;
    case ARGBEGIN:
        printf("ARGBEGIN");
        break;
    case ARG:
        printf("ARG");
        break;
    case CALL:
        printf("CALL");
        break;
    case RET:
        printf("RET");
        break;
    case CVTBS:
        printf("CVTBS");
        break;
    case CVTWS:
        printf("CVTWS");
        break;
    case CVTLS:
        printf("CVTLS");
        break;
    case CVTQS:
        printf("CVTQS");
        break;
    case CVTBU:
        printf("CVTBU");
        break;
    case CVTWU:
        printf("CVTWU");
        break;
    case CVTLU:
        printf("CVTLU");
        break;
    case CVTQU:
        printf("CVTQU");
        break;
    case CVTFL:
        printf("CVTFL");
        break;
    case CVTFQ:
        printf("CVTFQ");
        break;
    }
}

void print_quad(struct Quad *q) {
    printf("Quad: ");
    print_location(&q->eq);
    if (q->eq.loc_type != REG || q->eq.reg != EMPTY_VREG)
        printf(" = ");
    print_op(q->op);
    printf(" ");
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
        printf("\n");
    }
}
void print_bba(struct BasicBlockArr *bba) {
    size_t i;

    for (i = 0; i < bba->len; ++i) {
        printf("BasicBlock #%zu\n", i);
        print_bb(&bba->arr[i], 1);
    }
}
