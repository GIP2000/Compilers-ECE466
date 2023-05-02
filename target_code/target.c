#include "./target.h"
#include "../parser/symbol_table.h"
#include "../parser/types.h"
#include "../quads/ast_parser.h"
#include "../quads/quad.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define fprintft(fout, fmt, ...)                                               \
    fprintf((fout), "\t" fmt __VA_OPT__(, ) __VA_ARGS__)

extern struct VRegCounter v_reg_counter;

struct RealLocation {
    enum RealLocationType {
        TAGNAME,  // BSS or DATA
        REGISTER, // just a register
        CONST,
    } type;
    union {
        struct {
            char *tag_name;
            int is_owned;
        } tag;
        enum Registers reg;
        u64 constant;
    };
    int is_deref;
    int scale;
    i64 disp;
    int index_is_reg;
    union {
        enum Registers index_reg;
        u64 index_constant;
    };
};

int reg_live_map[REGISTERCOUNT] = {0, 0, 0, 0, 0, 0};

struct RealLocation get_available_scratch() {
    size_t i;
    for (i = 0; i < REGISTERCOUNT; ++i) {
        if (reg_live_map[i] == 0) {
            struct RealLocation rl;
            rl.type = REGISTER;
            rl.is_deref = 0;
            rl.reg = i + STARTREG;
            return rl;
        }
    }
    fprintf(stderr, "IMPLEMENT SPILL");
    exit(1);
}

void print_real_loc(FILE *fout, struct RealLocation rl);
void two_reg_print(FILE *fout, char *op, struct RealLocation arg1,
                   struct RealLocation arg2) {
    if (!(arg1.is_deref && arg2.is_deref)) {
        fprintft(fout, "%s\t", op);
        print_real_loc(fout, arg1);
        fprintf(fout, ", ");
        print_real_loc(fout, arg2);
        fprintf(fout, "\n");
        return;
    }
    fprintft(fout, "movl\t");
    print_real_loc(fout, arg1);
    fprintf(fout, ", ");
    struct RealLocation scratch = get_available_scratch();
    print_real_loc(fout, scratch);
    fprintf(fout, "\n");
    fprintft(fout, "%s\t", op);
    print_real_loc(fout, scratch);
    fprintf(fout, ", ");
    print_real_loc(fout, arg2);
    fprintf(fout, "\n");
}

char *get_str() {
    static size_t str_counter = 0;
    int length = str_counter == 0 ? 1 : log10(str_counter) + 1;
    char *name = (char *)malloc(sizeof(char) * (6 + length));
    sprintf(name, "$.LOS%zu", str_counter++);
    return name;
}

enum Registers pick_register(VReg v_reg) {
    if (v_reg_counter.arr[v_reg].real_reg != NONE) {
        enum Registers live_reg = v_reg_counter.arr[v_reg].real_reg;
        reg_live_map[live_reg - STARTREG] =
            (--v_reg_counter.arr[v_reg].count) > 0;
        return v_reg_counter.arr[v_reg].real_reg;
    }
    size_t i;
    for (i = 0; i < REGISTERCOUNT; ++i) {
        if (reg_live_map[i] == 0) {
            v_reg_counter.arr[v_reg].real_reg = i + STARTREG;
            reg_live_map[i] = (--v_reg_counter.arr[v_reg].count) > 0;
            return v_reg_counter.arr[v_reg].real_reg;
        }
    }
    // todo implment spill
    fprintf(stderr, "NOT ENOUGH LIVE REGISTERS\n");
    exit(1);
}

struct RealLocation convert_to_real(struct Location l) {
    struct RealLocation rl;
    rl.is_deref = l.deref;
    rl.scale = 1;
    rl.disp = 0;
    rl.index_constant = 0;
    rl.index_is_reg = 0;
    switch (l.loc_type) {
    case REG:
        rl.type = REGISTER;
        rl.reg = pick_register(l.reg);
    case VAR:
        if (l.var->symbol_loc == GLOBAL) {
            rl.type = TAGNAME;
            rl.tag.tag_name = l.var->name;
            rl.tag.is_owned = 0;
        } else {
            rl.type = REGISTER;
            rl.reg = EBP;
            rl.disp = l.var->offset;
        }
        break;
    case CONSTINT:
        rl.type = CONST;
        rl.constant = l.const_int;
        break;
    case CONSTFLOAT: {
        rl.type = CONST;
        rl.constant = *((i64 *)&l.const_float); // evil floating point bit hack
        break;
    }
    case BASICBLOCKNUM: {
        rl.type = TAGNAME;
        int length = l.bbn == 0 ? 1 : log10(l.bbn) + 1;
        //.BB = 3 0 at the end = 1
        rl.tag.tag_name = (char *)malloc((3 + 1 + length) * sizeof(char));
        rl.tag.is_owned = 1;
        sprintf(rl.tag.tag_name, ".BB%zu", l.bbn);
        break;
    }
    case CONSTSTR:
        rl.type = TAGNAME;
        rl.tag.tag_name = get_str();
        rl.tag.is_owned = 1;
        break;
    }

    return rl;
}

void get_reg_name(enum Registers reg, char *reg_name) {
    switch (reg) {
    case NONE:
        fprintf(stderr, "INTERNAL REGISTER ALLOCATION ERROR\n");
        exit(1);
    case SPILL:
        // todo IMPLEMENT
        return;
    case EBP:
        reg_name[4] = 0;
        reg_name[3] = 'p';
        reg_name[2] = 'b';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case EAX:
        reg_name[4] = 0;
        reg_name[3] = 'x';
        reg_name[2] = 'a';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case EDX:
        reg_name[4] = 0;
        reg_name[3] = 'x';
        reg_name[2] = 'd';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case ECX:
        reg_name[4] = 0;
        reg_name[3] = 'x';
        reg_name[2] = 'c';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case EBX:
        reg_name[4] = 0;
        reg_name[3] = 'x';
        reg_name[2] = 'b';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case ESI:
        reg_name[4] = 0;
        reg_name[3] = 'i';
        reg_name[2] = 's';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    case EDI:
        reg_name[4] = 0;
        reg_name[3] = 'i';
        reg_name[2] = 'd';
        reg_name[1] = 'e';
        reg_name[0] = '%';
        break;
    }
}

void print_real_loc(FILE *fout, struct RealLocation rl) {
    switch (rl.type) {
    case TAGNAME: {
        fprintf(fout, "%s", rl.tag.tag_name);
        if (rl.is_deref) {
            // TODO impl
            fprintf(fout, "(,");
            if (rl.index_is_reg) {
                char reg_name[5];
                get_reg_name(rl.reg, reg_name);
                fprintf(fout, "%s", reg_name);
            } else {
                fprintf(fout, "%lld", rl.index_constant);
            }
            fprintf(fout, ",%d)", rl.scale);
        }
        if (rl.tag.is_owned) {
            free(rl.tag.tag_name);
        }
        break;
    }
    case REGISTER: {
        char reg_name[5];
        get_reg_name(rl.reg, reg_name);
        if (rl.is_deref) {
            fprintf(fout, "%lld(%s, ", rl.disp, reg_name);
            if (rl.index_is_reg) {
                get_reg_name(rl.index_reg, reg_name);
                fprintf(fout, "%s", reg_name);
            } else {
                fprintf(fout, "%lld", rl.index_constant);
            }
            fprintf(fout, ",%d)", rl.scale);
        }
        break;
    }
    case CONST:
        fprintf(fout, "$%lld", rl.constant);
        break;
    }
}

static u64 unnamed_loc_val = 0;

static u64 last_arg_size = 0;

void initalize_data_and_bss_sections(FILE *fout,
                                     struct SymbolTable *global_table);
void output_basic_block(FILE *fout, struct BasicBlock *bb);
void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *func_type);

u64 get_alignment(struct Type *t);

void initalize_local_strs(FILE *fout, struct BasicBlockArr *bba) {
    size_t i;
    size_t str_counter = 0;
    int first = 1;
    for (i = 0; i < bba->len; ++i) {
        struct QuadListNode *current;
        for (current = bba->arr[i].head; current != NULL;
             current = current->next) {
            struct Quad q = current->quad;
            if (q.arg1.loc_type == CONSTSTR) {
                if (first)
                    fprintft(fout, ".section rodata\n");
                fprintf(fout, ".LOS%zu:\n", str_counter++);
                fprintft(fout, ".string\t\"%s\"\n", q.arg1.strlit.original_str);
            }
            if (q.arg2.loc_type == CONSTSTR) {
                if (first)
                    fprintft(fout, ".section rodata\n");
                fprintf(fout, ".LOS%zu:\n", str_counter++);
                fprintft(fout, ".string\t\"%s\"\n", q.arg2.strlit.original_str);
            }
        }
    }
}

void output_asm(char *output_file_name, struct BasicBlockArr *bba,
                struct SymbolTable *global_table) {

    FILE *fout = fopen(output_file_name, "w");

    initalize_data_and_bss_sections(fout, global_table);
    initalize_local_strs(fout, bba);
    fprintft(fout, ".text\n");
    size_t i;
    struct SymbolTableNode *last_ref = NULL;

    for (i = 0; i < bba->len; ++i) {
        if (bba->arr[i].ref != NULL && last_ref != bba->arr[i].ref) {
            initalize_function_and_locals(fout, bba->arr[i].ref->name,
                                          bba->arr[i].ref->val.type);
        }

        last_ref = bba->arr[i].ref;
        output_basic_block(fout, &bba->arr[i]);
    }

    fclose(fout);
}

void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *func_type) {

    fprintft(fout, ".globl\t%s\n", name);
    fprintft(fout, ".type\t%s, @function\n", name);
    fprintf(fout, "%s:\n", name);
    fprintft(fout, "pushl\t%%ebp\n");
    fprintft(fout, "movl\t%%esp, %%ebp\n");
    size_t i;
    i64 parameter_size = 8;
    // set offsets for parametrs
    for (i = 0; i < func_type->extentions.func.arg_count; ++i) {
        u64 size = size_of_abstract(
            func_type->extentions.func.scope->nodearr[i]->val.type);
        size = MAX(size, 4);
        func_type->extentions.func.scope->nodearr[i]->offset_marked = 1;
        func_type->extentions.func.scope->nodearr[i]->offset = parameter_size;
        parameter_size += size;
    }

    // set offsets for locals + calc stack grow size
    i64 total_size = 0;
    for (i = func_type->extentions.func.arg_count;
         i < func_type->extentions.func.scope->len; ++i) {
        func_type->extentions.func.scope->nodearr[i]->offset_marked = 1;
        u64 size = size_of_abstract(
            func_type->extentions.func.scope->nodearr[i]->val.type);
        u64 aligment = get_alignment(
            func_type->extentions.func.scope->nodearr[i]->val.type);
        if (total_size + size % aligment != 0) {
            total_size += aligment - size;
        }
        func_type->extentions.func.scope->nodearr[i]->offset =
            -(size + total_size);
        total_size += size;
    }
    if (total_size % 16 != 0) {
        total_size += 16 - total_size;
    }
    if (total_size > 0)
        fprintft(fout, "subl\t$%lld, %%esp\n", total_size);
}

void output_quad(FILE *fout, struct Quad *q) {
    static size_t arg_arr_size = 0;
    static struct RealLocation *arg_arr = NULL;

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
    case ARGBEGIN: {
        last_arg_size = 0;
        arg_arr_size = q->arg1.const_int;
        arg_arr = (struct RealLocation *)malloc(sizeof(struct RealLocation) *
                                                q->arg1.const_int);
        break;
    }
    case ARG: {
        last_arg_size += 4;
        arg_arr[q->arg1.const_int] = convert_to_real(q->arg2);
        break;
    }
    case CALL: {

        int i;
        for (i = (int)arg_arr_size - 1; i >= 0; --i) {
            fprintft(fout, "pushl\t");
            print_real_loc(fout, arg_arr[i]);
            fprintf(fout, "\n");
        }

        fprintft(fout, "call\t");
        struct RealLocation rl = convert_to_real(q->arg1);
        print_real_loc(fout, rl);
        fprintf(fout, "\n");
        fprintft(fout, "addl\t$%lld, %%esp\n", last_arg_size);

        arg_arr_size = 0;
        free(arg_arr);
        arg_arr = NULL;
        break;
    }
    case RET:
        // TODO make sure return value is in eax
        fprintft(fout, "leave\n");
        fprintft(fout, "ret\n");
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
        if (global_table->nodearr[i]->type != FUNCTION) {
            if (global_table->nodearr[i]->val.initalizer == NULL)
                initalize_bss_var(fout, global_table->nodearr[i]);
            else
                initalize_data_var(fout, global_table->nodearr[i]);
        }
    }
}
