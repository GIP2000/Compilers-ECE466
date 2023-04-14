#pragma once

#include "../parser/symbol_table.h"
#include <stddef.h>

typedef unsigned int VReg;
#define EMPTY_VREG 0
#define VREG_START 1
// number of the virtual register to use.
// the number 0 represents a null register
// ie there is no arg (b = a++ has one arg only) or eq (*d = 1; would not have
// an eq since its a STORE type command)

enum LocationType {
    REG,
    VAR,
    CONSTINT,
    CONSTFLOAT,
};

struct Location {
    enum LocationType loc_type; // 0 if reg 1 if var
    int deref;
    union {
        VReg reg;
        struct SymbolTableNode *var;
        double const_float;
        long long const_int;
    };
};

enum Operation {
    LOAD,
    STORE,
    LEA,
    MOV,
    // Math Ops
    ADD,
    SUB,
    MUL,
    DIV,
    FADD,
    FSUB,
    FMUL,
    FDIV,

    // BITWISE
    BINOT,

    // LOGICAL
    LOGNOT,
};

struct Quad {
    struct Location eq;
    enum Operation op;
    struct Location arg1;
    struct Location arg2;
};

struct QuadListNode {
    struct Quad quad;
    struct QuadListNode *next;
};

struct BasicBlock {
    struct QuadListNode *head;
    struct QuadListNode *tail;
    struct SymbolTableNode *ref;
};

struct BasicBlockArr {
    struct BasicBlock *arr;
    size_t cap;
    size_t len;
};

struct Location make_Location_int(long long v);
struct Location make_Location_float(double v);
struct Location make_Location_reg();
struct Location make_Location_empty_reg();
struct Location make_Location_var(struct SymbolTableNode *v);

struct BasicBlockArr initalize_BasicBlockArr(size_t cap);
void append_basic_block(struct BasicBlockArr *bba, struct BasicBlock bb);
void print_bba(struct BasicBlockArr *bba);

struct BasicBlock make_bb(struct SymbolTableNode *ref);

struct Quad make_quad(struct Location eq, enum Operation op,
                      struct Location arg1, struct Location arg2);
void append_quad(struct BasicBlock *bb, struct Quad quad);
void print_quad(struct Quad *q);
void print_bb(struct BasicBlock *bb, int add_tab);
void print_location(struct Location *l);
