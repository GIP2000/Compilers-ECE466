#include "./symbol_table.h"
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

void create_namespace() {
    struct SymbolTable *new = initalize_table(10);
    new->parent = symbol_table;
    symbol_table = new;
}

int find_in_table(char *name, enum Namespace namespc, struct SymbolTable *ct) {
    size_t i;
    for (i = 0; i < ct->len; ++i) {
        if (strcmp(name, ct->nodearr[i].name) &&
            ct->nodearr[i].namespc == namespc) {
            return 1;
        }
    }
    return 0;
}

// maybe change this to void and instead exit on error
int enter_in_namespace(char *name, enum Namespace namespc) {
    if (find_in_table(name, namespc, symbol_table)) {
        // handle specific cases like function prototypes ...
        return 0;
    }

    if (symbol_table->capacity - symbol_table->len <= 0) {
        // look into later if I should just be doing +1
        symbol_table->nodearr =
            realloc(symbol_table->nodearr, symbol_table->len + 10);
        symbol_table->capacity = symbol_table->len + 10;
    }

    struct SymbolTableNode *node = &symbol_table->nodearr[symbol_table->len++];
    node->name = name;
    node->namespc = namespc;

    return 1;
}

int find_in_namespace(char *name, enum Namespace namespc) {
    struct SymbolTable *ct = symbol_table;
    while (ct != NULL) {
        if (find_in_table(name, namespc, ct)) {
            return 1;
        }
        ct = ct->parent;
    }
    return 0;
}
