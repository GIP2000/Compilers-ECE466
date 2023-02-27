#pragma once
#include <stdlib.h>
enum IdentType { VARIABLE, FUNCTION, TYPEDEFNAME, TAG, LABELNAME, MEMBER };

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

enum StorageClass {
    S_EXTERN,
    S_AUTO,
    S_REG,
    S_STATIC,
};

enum Namespace {
    ORD,
    LABEL,
    MEMS,
    TAGS,
};

struct SymbolTable {
    struct SymbolTableNode *nodearr;
    size_t len;
    size_t capacity;
    struct SymbolTable *parent;
};

struct SymbolTableNode {
    char *name;
    enum Namespace namespc;
    enum IdentType type;
    struct {
        enum StorageClass sc;
        int qualifier_bit_mask;
        int function_spec_bit_mask;
        struct Type *type;
    } val;
};

struct SymbolTable *initalize_table(size_t capacity);

void create_namespace();

int find_in_namespace(char *name, enum Namespace namespc);

int enter_in_namespace(char *name, enum Namespace namespc);

void pop_symbol_table();
