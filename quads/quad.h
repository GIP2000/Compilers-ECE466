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

enum LocationType { REG, VAR, CONSTINT, CONSTFLOAT, BASICBLOCKNUM };

struct VRegCounter {
    size_t cap;
    struct VRegCounterNode {
        size_t count;
        int real_reg;
    } *arr;
};

struct Location {
    enum LocationType loc_type; // 0 if reg 1 if var
    int deref;
    union {
        VReg reg;
        struct SymbolTableNode *var;
        long double const_float;
        long long const_int;
        size_t bbn;
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
    MOD,

    // BITWISE
    BINOT,
    BIAND,
    BIOR,
    BIXOR,
    BISHL,
    BISHR,

    // LOGICAL
    CMP,
    BR,

    // branch
    BREQ,  // 0  0
    BRNEQ, // 1  2
    BRLT,  // 2  4
    BRLE,  // 3  6
    BRGT,  // 4  8
    BRGE,  // 5 10

    BREQU,  // 0  1
    BRNEQU, // 1  3
    BRLTU,  // 2  5
    BRLEU,  // 3  7
    BRGTU,  // 4  9
    BRGEU,  // 5 11

    // get rvalue
    CCEQ,  // 0  0
    CCNEQ, // 1  2
    CCLT,  // 2  4
    CCLE,  // 3  6
    CCGT,  // 4  8
    CCGE,  // 5 10

    CCEQU,  // 0  1
    CCNEQU, // 1  3
    CCLTU,  // 2  5
    CCLEU,  // 3  7
    CCGTU,  // 4  9
    CCGEU,  // 5 11

    //
    LOGNOT,
    // function stuff
    ARGBEGIN,
    ARG,
    CALL,
    RET,
    // Cast
    CVTBS,
    CVTWS,
    CVTLS,
    CVTQS,

    CVTBU,
    CVTWU,
    CVTLU,
    CVTQU,

    CVTFL,
    CVTFQ,

};

#define CMPLEN 6
#define CASTLEN 4

typedef int OpInverter[6];

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

void debug_print_vrc();
struct Location make_Location_int(long long v);
struct Location make_Location_float(long double v);
struct Location make_Location_reg();
struct Location make_Location_empty_reg();
struct Location make_Location_var(struct SymbolTableNode *v);
struct Location make_Location_BB(size_t bbn);

struct BasicBlockArr initalize_BasicBlockArr(size_t cap);
void append_basic_block(struct BasicBlockArr *bba, struct BasicBlock bb);
void print_bba(struct BasicBlockArr *bba);

struct BasicBlock make_bb(struct SymbolTableNode *ref);

struct Quad make_quad(struct Location eq, enum Operation op,
                      struct Location arg1, struct Location arg2);
struct Quad *append_quad(struct BasicBlock *bb, struct Quad quad);
void print_quad(struct Quad *q);
void print_bb(struct BasicBlock *bb, int add_tab);
void print_location(struct Location *l);
