#pragma once

#include "../parser/symbol_table.h"
#include <stddef.h>

// Virtual Register counter
typedef unsigned int VReg;

// number of the virtual register to use.
// the number 0 represents a null register
// ie there is no arg (b = a++ has one arg only) or eq (*d = 1; would not have
// an eq since its a STORE type command)
#define EMPTY_VREG 0
#define VREG_START 1

// Registers that we can use
enum Registers {
    // ------- CONTROL FLOW -------
    NONE = 0, // not assigned yet
    SPILL,    // if the register was spilled onto the stack
    EBP,      // register for bp (refrencing other local variables / parameters)
    // ------- CONTROL FLOW -------

    // ------- SCRATCH REGISTERS-------
    EAX, // acc register
    EDX, // data register
    ECX, // count register
    // ------- SCRATCH REGISTERS-------

    // ------- LONG TERM REGISTERS-------
    EBX, // base register
    ESI, // string source register
    EDI, // string dest register
         // ------- LONG TERM REGISTERS-------
    ESP
};
#define REGISTERCOUNT 6
#define STARTREG 3

enum LocationType { REG, VAR, CONSTINT, CONSTFLOAT, CONSTSTR, BASICBLOCKNUM };

struct VRegCounter {
    i64 cap;
    struct VRegCounterNode {
        size_t count;
        enum Registers real_reg;
        i64 spill_offset;
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
        YYlvalStrLit strlit;
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
    CCEQ,  // 0  1
    CCNEQ, // 1  0
    CCLT,  // 2  5
    CCLE,  // 3  4
    CCGT,  // 4  3
    CCGE,  // 5  2

    CCEQU,  // 0  1
    CCNEQU, // 1  3
    CCLTU,  // 2  5
    CCLEU,  // 3  7
    CCGTU,  // 4  9
    CCGEU,  // 5 11

    //
    // LOGNOT, REMOVED because it doesn't make sense
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
    VReg last_v_reg_used;
    VReg first_v_reg_used;
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
struct Location make_Location_str(YYlvalStrLit strlit);

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
