#pragma once
#include "ast.h"
#include <stdlib.h>
enum IdentType { VARIABLE, FUNCTION, TYPEDEFNAME, TAG, LABELNAME, MEMBER };

enum SymbolTableType { GLOBAL = 0, FUNC, PROTOTYPE, STRUCT_OR_UNION, BLOCK };

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
    ANY, // Only used for searching should never be assigned
};

struct DebugFileInfo {
    char *name;
    int ln;
};

struct SymbolTable {
    struct SymbolTableNode **nodearr;
    size_t len;
    size_t capacity;
    struct SymbolTable *parent;
    enum SymbolTableType st_type;
};

struct SymbolTableNode {
    char *name;
    enum Namespace namespc;
    enum IdentType type;
    struct DebugFileInfo fi;
    struct {
        enum StorageClass sc;
        struct Type *type;
        struct AstNode *initalizer;
    } val;
};
struct StNodeTablePair {
    struct SymbolTable *st;
    struct SymbolTableNode node;
};

struct StNodeTablePair make_st_node_pair(struct SymbolTableNode node);
struct StNodeTablePair make_st_node_pair_from(struct SymbolTable *st,
                                              struct SymbolTableNode node);
struct SymbolTableNode make_st_node(char *name, enum Namespace namespc,
                                    enum IdentType ident_type,
                                    enum StorageClass sc, struct Type *type,
                                    struct AstNode *initalizer);
void print_st(struct SymbolTable *st);

enum StorageClass get_default_sc();

struct SymbolTable *initalize_table(size_t capacity);

void create_scope(enum SymbolTableType type);

int find_in_namespace(char *name, enum Namespace namespc,
                      struct SymbolTableNode **output);

int enter_in_namespace(struct SymbolTableNode node, enum Namespace namespc);

void pop_global_table();

void pop_symbol_table();

int find_in_table(char *name, enum Namespace namespc, struct SymbolTable *ct,
                  struct SymbolTableNode **output);
struct SymbolTable *shallow_pop_table();
