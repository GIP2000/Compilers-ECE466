#include "./symbol_table.h"
#include "../lexer/file_info.h"
#include "./ast.h"
#include "./types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct SymbolTable *symbol_table;
extern void yyerror(const char *s);
extern FileInfo file_info;
extern int yylineno;

struct SymbolTable *initalize_table(size_t capacity) {
    struct SymbolTable *symbol_table =
        (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
    symbol_table->capacity = capacity;
    symbol_table->len = 0;
    symbol_table->nodearr = (struct SymbolTableNode **)malloc(
        sizeof(struct SymbolTableNode *) * capacity);

    return symbol_table;
}

void print_st(struct SymbolTable *st) {
    if (st == NULL) {
        printf("symbol table is null\n");
        return;
    } else {
        printf("st (%d): \n", st->st_type);
    }
    size_t i;
    for (i = 0; i < st->len; ++i) {
        // if (st->nodearr[i]->type != ORD)
        //     continue;
        printf(
            "%s: %d: name: %s, stroge duration: %d, storage linkage: %d: type ",
            st->nodearr[i]->fi.name, st->nodearr[i]->fi.ln,
            st->nodearr[i]->name, st->nodearr[i]->val.sc.sd,
            st->nodearr[i]->val.sc.sl);
        print_type(st->nodearr[i]->val.type);
        if (st->nodearr[i]->val.type->type == T_FUNC) {
            printf("{\n");
            print_AstNode(st->nodearr[i]->val.type->extentions.func.statment,
                          0);
            printf("}");
        }
        printf("\n");
    }
}

struct SymbolTable *shallow_pop_table() {
    struct SymbolTable *old = symbol_table;
    symbol_table = symbol_table->parent;
    return old;
}
void pop_global_table() {
    size_t i;
    for (i = 0; i < symbol_table->len; ++i) {
        free(symbol_table->nodearr[i]->name);
        free_type(symbol_table->nodearr[i]->val.type, 1);
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
                  struct SymbolTableNode **output) {
    if (name == NULL) {
        return 0;
    }
    size_t i;
    for (i = 0; i < ct->len; ++i) {
        if (ct->nodearr[i]->name == NULL)
            continue;
        if (strcmp(name, ct->nodearr[i]->name) == 0 &&
            (namespc == ANY || ct->nodearr[i]->namespc == namespc)) {
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

    while (namespc == LABEL && st->st_type != FUNC) {
        st = st->parent;
        if (st == NULL) {
            yyerror("Invalid Label Location");
            exit(2);
        }
    }
    struct SymbolTable *symbol_table = st;

    struct SymbolTableNode *n =
        (struct SymbolTableNode *)malloc(sizeof(struct SymbolTableNode));

    if (find_in_table(node.name, namespc, symbol_table, &n)) {
        // handle case of completing an incomplete struct
        if (namespc == TAGS && n->val.type->extentions.st_un.mem == NULL) {
            n->val.type->extentions.st_un.mem =
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

    *n = node;
    symbol_table->nodearr[symbol_table->len++] = n;

    return 1;
}

int find_in_namespace(char *name, enum Namespace namespc,
                      struct SymbolTableNode **output) {
    struct SymbolTable *ct = symbol_table;
    while (ct != NULL) {
        if ((namespc != LABEL || ct->st_type == FUNC) &&
            find_in_table(name, namespc, ct, output)) {
            return 1;
        }
        ct = ct->parent;
    }
    return 0;
}

struct EffectiveStorageClass get_default_sc() {
    return make_eff_storage_class(S_AUTO);
}

struct StNodeTablePair make_st_node_pair_from(struct SymbolTable *st,
                                              struct SymbolTableNode node) {
    struct StNodeTablePair r;
    r.st = st;
    r.node = node;
    return r;
}
struct StNodeTablePair make_st_node_pair(struct SymbolTableNode node) {
    struct StNodeTablePair r;
    r.st = symbol_table;
    r.node = node;
    return r;
}
struct EffectiveStorageClass make_eff_storage_class(enum StorageClass sc) {
    struct EffectiveStorageClass n;
    switch (sc) {
    case S_EXTERN:
        n.sd = SD_EXTERNAL;
        n.sl = SL_EXTERNAL;
        break;
    case S_STATIC:
        n.sd = SD_STATIC;
        n.sl = SL_INTERNAL;
        break;
    case S_REG:
        fprintf(stderr, "UNIMPLEMNTED");
        exit(2);
        break;
    case S_AUTO:
        if (symbol_table->st_type == GLOBAL) {
            n.sd = SD_STATIC;
            n.sl = SL_EXTERNAL;
        } else {
            n.sd = SD_AUTO;
            n.sl = SL_NONE;
        }
        break;
    default:
        fprintf(stderr, "UNRECHABLE Storage Class");
        exit(1);
    }
    return n;
}

struct SymbolTableNode make_st_node(char *name, enum Namespace namespc,
                                    enum IdentType ident_type,
                                    struct EffectiveStorageClass sc,
                                    struct Type *type,
                                    struct AstNode *initalizer) {
    struct SymbolTableNode n;
    n.name = name;
    n.offset = 0;
    n.offset_marked = 0;
    n.namespc = namespc;
    n.type = ident_type;
    n.val.initalizer = initalizer;
    n.val.type = type;
    n.val.sc = sc;
    n.fi.name = file_info.file_name;
    n.fi.ln = yylineno - file_info.real_line_start + file_info.file_line_start;
    return n;
}
