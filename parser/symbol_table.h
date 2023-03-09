#pragma once
#include <stdlib.h>
enum IdentType { VARIABLE, FUNCTION, TYPEDEFNAME, TAG, LABELNAME, MEMBER };

enum StorageClass {
    S_AUTO,
    S_EXTERN,
    S_REG,
    S_STATIC,
};

enum Namespace {
    ORD = 0,
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
        struct Type *type;
        struct AstNode *initalizer;
    } val;
};

struct SymbolTableNode make_st_node(char *name, enum Namespace namespc,
                                    enum IdentType ident_type,
                                    enum StorageClass sc, struct Type *type,
                                    struct AstNode *initalizer);

struct SymbolTable *initalize_table(size_t capacity);

void create_scope();

int find_in_namespace(char *name, enum Namespace namespc,
                      struct SymbolTableNode *output);

int enter_in_namespace(struct SymbolTableNode node, enum Namespace namespc);

void pop_symbol_table();
struct SymbolTable *shallow_pop_table();
