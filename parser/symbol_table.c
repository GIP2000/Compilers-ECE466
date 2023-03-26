#include "./symbol_table.h"
#include "./types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct SymbolTable *symbol_table;

struct SymbolTable *initalize_table(size_t capacity) {
    struct SymbolTable *symbol_table =
        (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
    symbol_table->capacity = capacity;
    symbol_table->len = 0;
    symbol_table->nodearr = (struct SymbolTableNode *)malloc(
        sizeof(struct SymbolTableNode) * capacity);

    return symbol_table;
}

struct SymbolTable *shallow_pop_table() {
    struct SymbolTable *old = symbol_table;
    symbol_table = symbol_table->parent;
    return old;
}
void pop_global_table() {
    size_t i;
    for (i = 0; i < symbol_table->len; ++i) {
        free(symbol_table->nodearr[i].name);
        free_type(symbol_table->nodearr[i].val.type, 1);
    }
    free(symbol_table->nodearr);

    // struct SymbolTable *old = symbol_table;
    // symbol_table = symbol_table->parent;
    struct SymbolTable *old = shallow_pop_table();
    free(old);
}

void pop_symbol_table() {
    if (symbol_table->parent == NULL) {
        fprintf(stderr, "Tried to pop global symbol table");
        exit(1);
    }
    pop_global_table();
}

void create_scope(enum SymbolTableType type) {
    struct SymbolTable *new = initalize_table(10);
    new->st_type = type;
    new->parent = symbol_table;
    symbol_table = new;
}

int find_in_table(char *name, enum Namespace namespc, struct SymbolTable *ct,
                  struct SymbolTableNode *output) {
    if (name == NULL) {
        return 0;
    }
    size_t i;
    for (i = 0; i < ct->len; ++i) {
        if (strcmp(name, ct->nodearr[i].name) == 0 &&
            ct->nodearr[i].namespc == namespc) {
            if (output != NULL)
                *output = ct->nodearr[i];
            return 1;
        }
    }
    return 0;
}

// maybe change this to void and instead exit on error
int enter_in_namespace(struct SymbolTableNode node, enum Namespace namespc) {

    struct SymbolTable *st = symbol_table;

    while (st->st_type == STRUCT_OR_UNION && namespc != MEMS) {
        st = st->parent;
    }
    struct SymbolTable *symbol_table = st;

    struct SymbolTableNode n;
    if (find_in_table(node.name, namespc, symbol_table, &n)) {
        // handle case of completing an incomplete struct
        if (namespc == TAGS && n.val.type->extentions.st_un.mem == NULL) {
            n.val.type->extentions.st_un.mem =
                node.val.type->extentions.st_un.mem;
            return 1;
        }
        return 0;
    }

    if ((long)symbol_table->capacity - (long)symbol_table->len <= 0) {
        fprintf(stderr, "realloc\n");
        // look into later if I should just be doing +1
        symbol_table->nodearr =
            realloc(symbol_table->nodearr,
                    sizeof(struct SymbolTableNode) * (symbol_table->len + 10));
        symbol_table->capacity = symbol_table->len + 10;
    }

    symbol_table->nodearr[symbol_table->len++] = node;

    return 1;
}

int find_in_namespace(char *name, enum Namespace namespc,
                      struct SymbolTableNode *output) {
    struct SymbolTable *ct = symbol_table;
    while (ct != NULL) {
        if (find_in_table(name, namespc, ct, output)) {
            return 1;
        }
        ct = ct->parent;
    }
    return 0;
}

enum StorageClass get_default_sc() {
    if (symbol_table->st_type == GLOBAL) {
        return S_STATIC;
    }
    return S_AUTO;
}

struct SymbolTableNode make_st_node(char *name, enum Namespace namespc,
                                    enum IdentType ident_type,
                                    enum StorageClass sc, struct Type *type,
                                    struct AstNode *initalizer) {
    struct SymbolTableNode n;
    n.name = name;
    n.namespc = namespc;
    n.type = ident_type;
    n.val.sc = sc;
    n.val.initalizer = initalizer;
    n.val.type = type;
    return n;
}
