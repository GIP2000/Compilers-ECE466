#include "./types.h"
#include "./symbol_table.h"
#include <stdio.h>
#include <string.h>

extern void yyerror(char *);

int types_eq(struct Type *t1, struct Type *t2) {
    if (t1->type != t2->type) {
        return 0;
    }

    if (t1->type == T_ARR && t1->extentions.next_type.arr_size_expression !=
                                 t2->extentions.next_type.arr_size_expression) {
        return 0;
    }

    if (t1->type >= T_POINTER && t1->type <= T_TYPEDEF) {
        return types_eq(t1->extentions.next_type.next,
                        t2->extentions.next_type.next);
    }

    if (t1->type == T_FUNC) {
        if (t1->extentions.func.arg_count != t2->extentions.func.arg_count ||
            t1->extentions.func.has_variable_args !=
                t2->extentions.func.has_variable_args ||
            !types_eq(t1->extentions.func.ret, t2->extentions.func.ret))
            return 0;

        size_t i;
        for (i = 0; i < t1->extentions.func.arg_count; ++i) {
            if (!types_eq(&t1->extentions.func.args[i],
                          &t2->extentions.func.args[i]))
                return 0;
        }
        return 1;
    }
    if (t1->type == T_STRUCT || t1->type == T_UNION) {
        if (t1->extentions.st_un.is_struct != t2->extentions.st_un.is_struct ||
            t1->extentions.st_un.mem->len != t2->extentions.st_un.mem->len)
            return 0;
        size_t i;
        for (i = 0; i < t1->extentions.st_un.mem->len; ++i) {
            if (t1->extentions.st_un.mem->nodearr[i].val.sc !=
                    t2->extentions.st_un.mem->nodearr[i].val.sc ||
                !types_eq(t1->extentions.st_un.mem->nodearr[i].val.type,
                          t2->extentions.st_un.mem->nodearr[i].val.type))
                return 0;
            ;
        }
        return 1;
    }
    return 1;
}

int func_is_comp(struct Type *old_node, struct Type *current_node) {
    // This does not follow the exact rules, it just makes sure they are exactly
    // equal but allows f() -> f(details)
    if (old_node->extentions.func.statment != NULL) {
        return 0;
    }
    if (old_node->extentions.func.args == NULL &&
        old_node->extentions.func.has_variable_args ==
            current_node->extentions.func.has_variable_args &&
        types_eq(old_node->extentions.func.ret,
                 current_node->extentions.func.ret)) {
        old_node->extentions.func.args = current_node->extentions.func.args;
        old_node->extentions.func.arg_count =
            current_node->extentions.func.arg_count;
    }

    return types_eq(old_node, current_node);
}
struct Type *add_to_end_and_reverse(struct Type *source, struct Type *end) {
    if (end == NULL) {
        return source;
    }
    if (source == NULL) {
        return end;
    }
    if (source->type == T_FUNC) {
        struct Type *t =
            add_to_end_and_reverse(source->extentions.func.ret, end);
        source->extentions.func.ret = t;

    } else if (source->type >= T_POINTER && source->type <= T_TYPEDEF) {
        struct Type *t = reverse_next(source);
        struct Type *last = get_last_from_next(t);

        if (last->type >= T_POINTER && last->type <= T_TYPEDEF &&
            last->extentions.next_type.next == NULL) {
            last->extentions.next_type.next = end;
            return t;
        }
        // DEBUG REMOVE LATER
        if (last->type >= T_POINTER && last->type <= T_TYPEDEF &&
            last->extentions.next_type.next != NULL) {
            fprintf(stderr, "ERROR should never happen \n");
        }
        add_to_end_and_reverse(last, end);
        return t;
    }

    return source;
}

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
        new_type->extentions.st_un.is_struct = type->extentions.st_un.is_struct;
        new_type->extentions.st_un.mem =
            (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
        memcpy(new_type->extentions.st_un.mem, type->extentions.st_un.mem,
               sizeof(struct SymbolTable));
        new_type->extentions.st_un.mem->nodearr =
            (struct SymbolTableNode *)malloc(
                sizeof(struct SymbolTableNode) *
                type->extentions.st_un.mem->capacity);
        memcpy(new_type->extentions.st_un.mem->nodearr,
               type->extentions.st_un.mem->nodearr,
               sizeof(struct SymbolTableNode) *
                   type->extentions.st_un.mem->capacity);
        size_t i;
        for (i = 0; i < new_type->extentions.st_un.mem->len; ++i) {
            new_type->extentions.st_un.mem->nodearr[i].val.type =
                clone_type(new_type->extentions.st_un.mem->nodearr[i].val.type);
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
        for (i = 0; i < type->extentions.st_un.mem->len; ++i) {
            free_type(type->extentions.st_un.mem->nodearr[i].val.type, 1);
        }
        free(type->extentions.st_un.mem->nodearr);
    }
    if (free_end) {
        free(type);
    }
}

struct Type *make_default_type(enum Types type) {

    struct Type *type_obj = (struct Type *)malloc(sizeof(struct Type));
    type_obj->type = type;
    type_obj->qualifier_bit_mask = 0;
    return type_obj;
}

struct Type *make_next_type(enum Types type, struct Type *next) {
    struct Type *type_obj = make_default_type(type);
    if (type == T_ARR && next != NULL && next->type == T_FUNC) {
        yyerror("Can't make array of functions");
        exit(2);
    }

    type_obj->extentions.next_type.next = next;
    return type_obj;
}

struct Type *make_func_type(struct Type *ret, struct SymbolTable *pt,
                            int has_variable_args) {
    struct Type *type_obj = make_default_type(T_FUNC);
    type_obj->extentions.func.has_variable_args = has_variable_args;
    type_obj->extentions.func.statment = NULL;
    type_obj->extentions.func.ret = ret != NULL ? clone_type(ret) : NULL;
    if (pt == NULL) {
        type_obj->extentions.func.arg_count = 0;
        type_obj->extentions.func.args = NULL;
        return type_obj;
    }
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

struct Type *reverse_and_merge(struct Type *first, struct Type *second) {
    struct Type *prev = NULL;
    struct Type *next = NULL;
    struct Type *current;
    for (current = second; current != NULL && current->type >= T_POINTER &&
                           current->type <= T_TYPEDEF;
         current = next) {
        next = current->extentions.next_type.next;
        current->extentions.next_type.next = prev;
        prev = current;
    }

    for (current = first; current != NULL && current->type >= T_POINTER &&
                          current->type <= T_TYPEDEF;
         current = next) {
        next = current->extentions.next_type.next;
        current->extentions.next_type.next = prev;
        prev = current;
    }

    if (current != NULL) {
        print_type(current);
        printf("\n");
        get_last_from_next(prev)->extentions.next_type.next = current;
    }
    return prev;
}

struct Type *reverse_next(struct Type *start) {
    if (start == NULL || start->type < T_POINTER || start->type > T_TYPEDEF) {
        return start;
    }
    struct Type *prev = NULL;
    struct Type *next = NULL;
    struct Type *current;
    for (current = start; current != NULL && current->type >= T_POINTER &&
                          current->type <= T_TYPEDEF;
         current = next) {
        next = current->extentions.next_type.next;
        current->extentions.next_type.next = prev;
        prev = current;
    }
    if (current != NULL && start != NULL && next != NULL) {
        // start should be last here
        // next should have your extra stuff
        start->extentions.next_type.next = next;
    }
    return prev;
}

struct Type *merge_if_next(struct Type *parent, struct Type *child) {
    if (parent == NULL) {
        return child;
    }

    if (parent->type >= T_POINTER && parent->type <= T_TYPEDEF) {
        parent->extentions.next_type.next = child;
        return parent;
    }
    fprintf(stderr, "This should never happen"); // this might be a syntax
                                                 // error
    // I should check
    exit(2);
}

struct Type *get_last_from_next(struct Type *t) {
    struct Type *start;
    if (t->type == T_FUNC) {
        return get_last_from_next(t->extentions.func.ret);
    }
    for (start = t; start->extentions.next_type.next != NULL &&
                    start->type >= T_POINTER && start->type <= T_TYPEDEF;
         start = start->extentions.next_type.next) {
    }
    return start;
}

void add_or_throw_type(struct Type *parent, struct Type *child) {
    if (!(parent->type >= T_POINTER && parent->type <= T_TYPEDEF)) {
        yyerror("Invalid Type Defintion");
        exit(2);
    }
    parent->extentions.next_type.next = child;
}

struct Type *make_struct_or_union(int is_struct, struct SymbolTable *mem) {
    struct Type *t = make_default_type(T_STRUCT);
    t->extentions.st_un.is_struct = is_struct;
    t->extentions.st_un.mem = mem;
    return t;
}

void print_type_i(struct Type *type, int prev_pointer) {

    switch (type->type) {

    case T_VOID:
        printf("VOID");
        break;
    case T_SHORT:
        printf("SHORT");
        break;
    case T_INT:
        printf("INT");
        break;
    case T_CHAR:
        printf("CHAR");
        break;
    case T_FLOAT:
        printf("FLOAT");
        break;
    case T_DOUBLE:
        printf("DOUBLE");
        break;
    case T_POINTER:
        printf("POINTER");
        break; // next range
    case T_SIGNED:
        printf("SIGNED");
        break;
    case T_UNSIGNED:
        printf("UNSIGNED");
        break;
    case T_LONG:
        printf("LONG");
        break;
    case T_ARR:
        printf("ARR");
        break;
    case T_TYPEDEF:
        printf("TYPEDEF");
        break; // end next range
    case T_FUNC:
        printf("FUNC with %zu args", type->extentions.func.arg_count);
        break;
    case T_STRUCT:
        printf("STRUCT");
        break;
    case T_UNION:
        printf("UNION");
        break;
    case T_ENUM:
        printf("ENUM");
        break;
    default:
        fprintf(stderr, "Critical Error Invalid Type");
        exit(1);
    }

    printf(" qualifer: (%d)", type->qualifier_bit_mask);

    if (type->type < T_POINTER) {
        return;
    }

    if (type->type < T_TYPEDEF) {
        printf(" -> ");
        if (type->extentions.next_type.next != NULL) {
            print_type_i(type->extentions.next_type.next, 1);
        }
        return;
    }

    if (type->type == T_FUNC) {
        printf(" ret: ");
        if (type->extentions.func.ret == NULL)
            printf("Unkown");
        else
            print_type_i(type->extentions.func.ret, 0);
        printf(" args: (");
        size_t i;
        for (i = 0; i < type->extentions.func.arg_count; ++i) {
            print_type_i(&type->extentions.func.args[i], 0);
            printf(", ");
        }
        printf(")");
    }

    if (type->type == T_STRUCT && !prev_pointer) {
        printf(" members: (");
        size_t i;
        for (i = 0; i < type->extentions.st_un.mem->len; ++i) {
            print_type_i(type->extentions.st_un.mem->nodearr[i].val.type, 0);
            if (type->extentions.st_un.mem->nodearr[i].name != NULL) {
                printf(" %s", type->extentions.st_un.mem->nodearr[i].name);
            }
            printf(", ");
        }
        printf(")");
    }
}

void print_type(struct Type *type) { print_type_i(type, 0); }
