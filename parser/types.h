#include "./symbol_table.h"
#include <stdlib.h>
enum Types {
    T_SHORT,
    T_INT,
    T_LONG,
    T_LONGLONG,
    T_USHORT,
    T_UINT,
    T_ULONG,
    T_ULONGLONG,
    T_CHAR,
    T_UCHAR,
    T_FLOAT,
    T_DOUBLE,
    T_LONGDOUBLE,
    T_POINTER,
    T_ARR,
    T_TYPEDEF,
    T_FUNC,
    T_STRUCT,
    T_UNION,
    T_ENUM

};

struct Type {
    enum Types type;
    union {
        struct {
            struct Type *next;
        } next_type;
        struct {
            struct Type *ret;
            size_t arg_count;
            struct Type *args;
        } func;
        struct {
            struct SymbolTable mem;
        } st_un;
    } extentions;
};

void free_type(struct Type *type, int free_end);

struct Type *make_default_type(enum Types type);
struct Type *make_next_type(enum Types type, struct Type *next);

struct Type *make_func_type(struct Type *ret, struct SymbolTable *pt);
