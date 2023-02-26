#pragma once
#include <stdlib.h>
enum IdentType {
    VARIABLE,
    FUNCTION,
    TAG,
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

union SymbolVal {
    struct {
    } var;
    struct {
    } func;
    struct {
        struct SymbolTable mem;
    } tag;
    struct {
    } label;
    struct {
    } typedef_name;
};

struct SymbolTableNode {
    char *name;
    enum Namespace namespc;
    enum IdentType type;
    union SymbolVal val;
};

struct SymbolTable *initalize_table(size_t capacity);

void create_namespace();

int find_in_namespace(char *name, enum Namespace namespc);

int enter_in_namespace(char *name, enum Namespace namespc);
