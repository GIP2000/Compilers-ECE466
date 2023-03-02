#include "./symbol_table.h"
#include <stdlib.h>

enum TypeQualifier {
    Q_CONST = 0,    // 000
    Q_RESTRICT = 1, // 001
    Q_VOLATILE = 2, // 010
    Q__ATOMIC = 4,  // 100
};

enum FunctionSpecifier {
    F_INLINE = 0,
    F__NORETURN = 1,
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

struct Type *merge_if_next(struct Type *parent, struct Type *chlid);

void free_type(struct Type *type, int free_end);

void add_or_throw_type(struct Type *parent, struct Type *child);

struct Type *make_default_type(enum Types type);
struct Type *make_next_type(enum Types type, struct Type *next);

struct Type *make_func_type(struct Type *ret, struct SymbolTable *pt);
