#include "./symbol_table.h"
#include <stdlib.h>

enum TypeQualifier {
    Q_NONE = 0,     // 0000
    Q_CONST = 1,    // 0001
    Q_RESTRICT = 2, // 0010
    Q_VOLATILE = 4, // 0100
    Q__ATOMIC = 8,  // 1000
};

enum FunctionSpecifier {
    F_NONE = 0,      // 00
    F_INLINE = 1,    // 01
    F__NORETURN = 2, // 10
};

enum Types {
    T_VOID,
    T_SHORT,
    T_INT,
    T_CHAR,
    T_FLOAT,
    T_DOUBLE,
    T_POINTER, // next range
    T_SIGNED,
    T_UNSIGNED,
    T_LONG,
    T_ARR,
    T_TYPEDEF, // end next range
    T_FUNC,
    T_STRUCT,
    T_UNION,
    T_ENUM

};

struct Type {
    enum Types type;
    int qualifier_bit_mask;
    union {
        struct {
            struct Type *next;
            struct AstNode *arr_size_expression;
        } next_type;
        struct {
            int function_spec_bit_mask;
            struct Type *ret;

            size_t arg_count;
            struct Type *args;
        } func;
        struct {
            struct SymbolTable mem;
        } st_un;
    } extentions;
};

void print_type(struct Type *type);

struct Type *merge_if_next(struct Type *parent, struct Type *chlid);

void free_type(struct Type *type, int free_end);

void add_or_throw_type(struct Type *parent, struct Type *child);

struct Type *make_default_type(enum Types type);
struct Type *make_next_type(enum Types type, struct Type *next);
struct Type *reverse_next(struct Type *start);

struct Type *make_func_type(struct Type *ret, struct SymbolTable *pt);
