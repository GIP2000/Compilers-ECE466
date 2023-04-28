#include "../parser/symbol_table.h"
#include "../parser/types.h"
#include "../quads/quad.h"
#include <stdio.h>

extern struct VRegCounter v_reg_counter;

void initalize_data_section(FILE *fout, struct SymbolTable *global_table);
void output_basic_block(FILE *fout, struct BasicBlock *bb);
void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *type_arr, size_t arr_len);

void output_asm(char *output_file_name, struct BasicBlockArr *bba,
                struct SymbolTable *global_table) {

    FILE *output_file = fopen(output_file_name, "w");

    initalize_data_section(output_file, global_table);
    size_t i;
    struct SymbolTableNode *last_ref = NULL;

    for (i = 0; i < bba->len; ++i) {
        if (last_ref != bba->arr[i].ref) {
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

void initalize_data_section(FILE *fout, struct SymbolTable *global_table) {
    size_t i;
    for (i = 0; i < global_table->len; ++i) {
        if (global_table->nodearr[i]->type != FUNCTION ||
            global_table->nodearr[i]->val.type->extentions.func.statment ==
                NULL) {
            fprintf(stderr, "GLOBALS NOT SUPPORTED\n");
            exit(4);
        }
    }
}
