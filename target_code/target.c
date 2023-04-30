#include "../parser/symbol_table.h"
#include "../parser/types.h"
#include "../quads/ast_parser.h"
#include "../quads/quad.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern struct VRegCounter v_reg_counter;

static u64 unnamed_loc_val = 0;

#define fprintft(fout, fmt, ...)                                               \
    fprintf((fout), "\t" fmt __VA_OPT__(, ) __VA_ARGS__)

void initalize_data_and_bss_sections(FILE *fout,
                                     struct SymbolTable *global_table);
void output_basic_block(FILE *fout, struct BasicBlock *bb);
void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *type_arr, size_t arr_len);

void output_asm(char *output_file_name, struct BasicBlockArr *bba,
                struct SymbolTable *global_table) {

    FILE *output_file = fopen(output_file_name, "w");

    initalize_data_and_bss_sections(output_file, global_table);
    size_t i;
    struct SymbolTableNode *last_ref = NULL;

    for (i = 0; i < bba->len; ++i) {
        if (bba->arr[i].ref != NULL && last_ref != bba->arr[i].ref) {
            initalize_function_and_locals(
                output_file, bba->arr[i].ref->name,
                bba->arr[i].ref->val.type->extentions.func.args,
                bba->arr[i].ref->val.type->extentions.func.arg_count);
        }

        last_ref = bba->arr[i].ref;
        output_basic_block(output_file, &bba->arr[i]);
    }

    fclose(output_file);
}

void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *type_arr, size_t arr_len) {

    fprintft(fout, ".globl\t%s\n", name);
    fprintft(fout, ".type\t%s, @function\n", name);
    fprintf(fout, "%s:\n", name);
    // TODO initalize locals and set offsets
}

void output_quad(FILE *fout, struct Quad *q) {
    switch (q->op) {
    case LOAD:
        break;
    case STORE:
        break;
    case LEA:
        break;
    case MOV:
        break;
    case ADD:
        break;
    case SUB:
        break;
    case MUL:
        break;
    case DIV:
        break;
    case FADD:
        break;
    case FSUB:
        break;
    case FMUL:
        break;
    case FDIV:
        break;
    case MOD:
        break;
    case BINOT:
        break;
    case BIAND:
        break;
    case BIOR:
        break;
    case BIXOR:
        break;
    case BISHL:
        break;
    case BISHR:
        break;
    case CMP:
        break;
    case BR:
        break;
    case BREQ:
        break;
    case BRNEQ:
        break;
    case BRLT:
        break;
    case BRLE:
        break;
    case BRGT:
        break;
    case BRGE:
        break;
    case BREQU:
        break;
    case BRNEQU:
        break;
    case BRLTU:
        break;
    case BRLEU:
        break;
    case BRGTU:
        break;
    case BRGEU:
        break;
    case CCEQ:
        break;
    case CCNEQ:
        break;
    case CCLT:
        break;
    case CCLE:
        break;
    case CCGT:
        break;
    case CCGE:
        break;
    case CCEQU:
        break;
    case CCNEQU:
        break;
    case CCLTU:
        break;
    case CCLEU:
        break;
    case CCGTU:
        break;
    case CCGEU:
        break;
    case LOGNOT:
        break;
    case ARGBEGIN:
        break;
    case ARG:
        break;
    case CALL:
        break;
    case RET:
        break;
    case CVTBS:
        break;
    case CVTWS:
        break;
    case CVTLS:
        break;
    case CVTQS:
        break;
    case CVTBU:
        break;
    case CVTWU:
        break;
    case CVTLU:
        break;
    case CVTQU:
        break;
    case CVTFL:
        break;
    case CVTFQ:
        break;
    }
}

void output_basic_block(FILE *fout, struct BasicBlock *bb) {
    struct QuadListNode *quad_list;
    for (quad_list = bb->head; quad_list != NULL; quad_list = quad_list->next) {
        output_quad(fout, &quad_list->quad);
    }
}

u64 get_alignment(struct Type *t) {
    u64 alignment;
    if (t->type == T_STRUCT) {
        if (!t->extentions.st_un.is_cached) {
            size_of_abstract(t);
        }
        return t->extentions.st_un.alignment_size;
    }
    struct Type *current_t = t;
    while (current_t->type == T_ARR) {
        current_t = current_t->extentions.next_type.next;
    }
    return size_of_abstract(current_t);
}

void initalize_bss_var(FILE *fout, struct SymbolTableNode *node) {
    if (node->val.sc.sd == SD_EXTERNAL) {
        return;
    }
    if (node->val.sc.sl == SL_INTERNAL) {
        fprintft(fout, ".local\n");
    }
    fprintft(fout, ".comm,%lld,%lld\n", get_alignment(node->val.type),
             size_of_abstract(node->val.type));
}

void initalize_data_var(FILE *fout, struct SymbolTableNode *node) {
    if (node->val.sc.sd == SD_EXTERNAL) {
        return;
    }
    if (node->val.sc.sl != SL_INTERNAL) {
        fprintft(fout, ".globl\t%s\n", node->name);
    }
    if (node->val.type->type == T_POINTER) {
        // its a string
        fprintft(fout, ".section\t.rodata\n");
        fprintf(fout, ".LC%lld:\n", unnamed_loc_val);
        fprintft(fout, ".string\t%s\n",
                 node->val.initalizer->strlit.original_str);
    }
    fprintft(fout, ".type\t%s, @object\n", node->name);
    fprintft(fout, ".data\n");
    u64 alignment = get_alignment(node->val.type);
    fprintft(fout, ".align %lld\n", alignment);
    u64 size = size_of_abstract(node->val.type);
    fprintft(fout, ".size %lld\n", size);
    fprintf(fout, "%s:\n", node->name);
    if (node->val.type->type == T_POINTER) {
        fprintft(fout, ".long\t.LC%lld", unnamed_loc_val++);
        return;
    }
    if (node->val.type->type == T_CHAR) {
        fprintft(fout, ".byte\t%c\n", node->val.initalizer->constant.val.chr);
    }
    if (size == 2) {
        fprintft(fout, ".value\t%lld\n",
                 node->val.initalizer->constant.type > TLONGLONG
                     ? node->val.initalizer->constant.val.u_int
                     : (i64)node->val.initalizer->constant.val.u_int);
    } else if (size == 4) {
        fprintft(fout, ".long\t%lld\n",
                 node->val.initalizer->constant.type > TLONGLONG
                     ? node->val.initalizer->constant.val.u_int
                     : (i64)node->val.initalizer->constant.val.u_int);
    } else if (size == 8) {
        u32 first =
            (node->val.initalizer->constant.val.u_int >> 4) & 0x0000FFFF;
        u32 second = node->val.initalizer->constant.val.u_int & 0x0000FFFF;
        fprintft(fout, ".long\t%lld\n",
                 node->val.initalizer->constant.type > TLONGLONG ? first
                                                                 : (i64)first);
        fprintft(fout, ".long\t%lld\n",
                 node->val.initalizer->constant.type > TLONGLONG ? second
                                                                 : (i64)second);
    }
}

void initalize_data_and_bss_sections(FILE *fout,
                                     struct SymbolTable *global_table) {
    size_t i;
    for (i = 0; i < global_table->len; ++i) {
        if (global_table->nodearr[i]->type != FUNCTION ||
            global_table->nodearr[i]->val.type->extentions.func.statment ==
                NULL) {
            if (global_table->nodearr[i]->val.initalizer == NULL)
                initalize_bss_var(fout, global_table->nodearr[i]);
            else
                initalize_data_var(fout, global_table->nodearr[i]);
        }
    }
}
