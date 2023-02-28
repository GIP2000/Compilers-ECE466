#include "./types.h"
#include "./symbol_table.h"
#include <stdio.h>
#include <string.h>

extern void yyerror(char *);

struct Type *clone_type(struct Type *type) {
    struct Type *new_type = (struct Type *)malloc(sizeof(struct Type));
    new_type->type = type->type;

    if (new_type->type >= T_POINTER && new_type->type <= T_TYPEDEF) {
        new_type->extentions.next_type.next =
            clone_type(type->extentions.next_type.next);
    } else if (new_type->type == T_FUNC) {
        new_type->extentions.func.ret = clone_type(type->extentions.func.ret);
        new_type->extentions.func.arg_count = type->extentions.func.arg_count;
        new_type->extentions.func.args = (struct Type *)malloc(
            sizeof(struct Type) * type->extentions.func.arg_count);
        size_t i;
        for (i = 0; i < type->extentions.func.arg_count; ++i) {
            struct Type *temp = clone_type(&type->extentions.func.args[i]);
            new_type->extentions.func.args[i] = *temp;
            free(temp); // this is ugly but also make sure this works;
        }
    } else if (new_type->type == T_STRUCT || new_type->type == T_UNION) {
        new_type->extentions.st_un.mem = type->extentions.st_un.mem;
        memcpy(new_type->extentions.st_un.mem.nodearr,
               type->extentions.st_un.mem.nodearr,
               sizeof(struct SymbolTableNode) *
                   type->extentions.st_un.mem.capacity);
        size_t i;
        for (i = 0; i < new_type->extentions.st_un.mem.len; ++i) {
            new_type->extentions.st_un.mem.nodearr[i].val.type =
                clone_type(new_type->extentions.st_un.mem.nodearr[i].val.type);
        }
    }
    return new_type;
}

void free_type(struct Type *type, int free_end) {
    if (type->type >= T_POINTER && type->type <= T_TYPEDEF) {
        free_type(type->extentions.next_type.next, 1);
    } else if (type->type == T_FUNC) {
        free_type(type->extentions.func.ret, 1);
        size_t i;
        for (i = 0; i < type->extentions.func.arg_count; ++i) {
            free_type(&type->extentions.func.args[i], 0);
        }
        free(type->extentions.func.args);
    } else if (type->type == T_STRUCT || type->type == T_UNION) {
        size_t i;
        for (i = 0; i < type->extentions.st_un.mem.len; ++i) {
            free_type(type->extentions.st_un.mem.nodearr[i].val.type, 1);
        }
        free(type->extentions.st_un.mem.nodearr);
    }
    if (free_end) {
        free(type);
    }
}

struct Type *make_default_type(enum Types type) {

    struct Type *type_obj = (struct Type *)malloc(sizeof(struct Type));
    type_obj->type = type;
    return type_obj;
}

struct Type *make_next_type(enum Types type, struct Type *next) {
    struct Type *type_obj = make_default_type(type);
    type_obj->extentions.next_type.next = next;
    return type_obj;
}

struct Type *make_func_type(struct Type *ret, struct SymbolTable *pt) {
    struct Type *type_obj = make_default_type(T_FUNC);
    type_obj->extentions.func.ret = clone_type(ret);
    type_obj->extentions.func.arg_count = pt->len;
    type_obj->extentions.func.args =
        (struct Type *)malloc(sizeof(struct Type) * pt->len);
    size_t i;
    for (i = 0; i < pt->len; ++i) {
        struct Type *temp = clone_type(pt->nodearr[i].val.type);
        type_obj->extentions.func.args[i] = *temp;
        free(temp);
    }
    // should be able to run pop on the symbol table now

    return type_obj;
}

struct Type *merge_if_next(struct Type *parent, struct Type *child) {
    if (parent == NULL) {
        return child;
    }

    if (parent->type >= T_POINTER && parent->type <= T_TYPEDEF) {
        parent->extentions.next_type.next = child;
        return parent;
    }
    fprintf(stderr, "This should never happen"); // this might be a syntax error
                                                 // I should check
    exit(2);
}

void add_or_throw_type(struct Type *parent, struct Type *child) {
    if (!(parent->type >= T_POINTER && parent->type <= T_TYPEDEF)) {
        yyerror("Invalid Type Defintion");
        exit(2);
    }
    parent->extentions.next_type.next = child;
}
