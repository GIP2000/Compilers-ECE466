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

void create_scope() {
    struct SymbolTable *new = initalize_table(10);
    new->parent = symbol_table;
    symbol_table = new;
}

int find_in_table(char *name, enum Namespace namespc, struct SymbolTable *ct,
                  struct SymbolTableNode *output) {
    if (name == NULL) {
        // if the name is null then It defaults to go in the symbol table since
        // it is anonymous
        return 1;
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

    if (find_in_table(node.name, namespc, symbol_table, NULL)) {
        // handle specific cases like function prototypes ...
        return 0;
    }
    // fprintf(stderr, "before -> cap: %ld , len: %ld add: %p\n",
    //         (long)(symbol_table->capacity), (long)(symbol_table->len),
    //         symbol_table->nodearr);

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
