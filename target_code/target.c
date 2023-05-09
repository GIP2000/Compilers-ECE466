#include "./target.h"
#include "../parser/symbol_table.h"
#include "../parser/types.h"
#include "../quads/ast_parser.h"
#include "../quads/quad.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
    int deref_overflow;
    int is_deref;
    int scale;
    i64 disp;
    int index_is_reg;
    union {
        enum Registers index_reg;
        u64 index_constant;
    };
};

struct RealLocation make_constant(u64 constant) {
    struct RealLocation rl;
    rl.type = CONST;
    rl.constant = constant;
    rl.is_deref = 0;
    return rl;
}

int reg_live_map[REGISTERCOUNT] = {0, 0, 0, 0, 0, 0};

VReg reg_to_vreg_map[REGISTERCOUNT] = {0, 0, 0, 0, 0, 0};

struct RealLocation get_register(enum Registers reg) {
    if (reg_live_map[reg - STARTREG]) {
        fprintf(stderr, "REGISTER IN USE FOR SOME REASON should spill it");
        exit(1);
    }
    struct RealLocation rl;
    rl.type = REGISTER;
    rl.is_deref = 0;
    rl.deref_overflow = 0;
    rl.reg = reg;
    rl.scale = 1;

    return rl;
}

enum Registers pick_register(VReg v_reg, int *is_spill) {
    *is_spill = 0;
    if (v_reg_counter.arr[v_reg].real_reg != NONE) {
        enum Registers live_reg = v_reg_counter.arr[v_reg].real_reg;
        if (live_reg == SPILL) {
            *is_spill = 1;
            return live_reg;
        }
        reg_live_map[live_reg - STARTREG] =
            (--v_reg_counter.arr[v_reg].count) > 0;
        if (reg_live_map[live_reg - STARTREG] == 0) {
            reg_to_vreg_map[live_reg - STARTREG] = 0;
        }
        return v_reg_counter.arr[v_reg].real_reg;
    }
    size_t i;
    for (i = 0; i < REGISTERCOUNT; ++i) {
        if (reg_live_map[i] == 0) {
            v_reg_counter.arr[v_reg].real_reg = i + STARTREG;
            reg_live_map[i] = (--v_reg_counter.arr[v_reg].count) > 0;
            if (reg_live_map[i] != 0) {
                reg_to_vreg_map[i] = v_reg;
            }
            return v_reg_counter.arr[v_reg].real_reg;
        }
    }
    // todo implment spill
    fprintf(stderr, "NOT ENOUGH LIVE REGISTERS\n");
    exit(1);
}
void spill_register(VReg v_reg, i64 spill_offset) {
    v_reg_counter.arr[v_reg].real_reg = SPILL;
    v_reg_counter.arr[v_reg].spill_offset = spill_offset;
}

struct RealLocation get_available_scratch() {
    size_t i;
    for (i = 0; i < REGISTERCOUNT; ++i) {
        if (reg_live_map[i] == 0) {
            struct RealLocation rl;
            rl.type = REGISTER;
            rl.is_deref = 0;
            rl.reg = i + STARTREG;
            rl.deref_overflow = 0;
            rl.scale = 1;
            rl.disp = 0;
            rl.index_is_reg = 0;
            rl.index_constant = 0;
            reg_live_map[i] = 1;
            return rl;
        }
    }
    fprintf(stderr, "IMPLEMENT SPILL");
    exit(1);
}

void kill_reg(enum Registers reg) { reg_live_map[reg - STARTREG] = 0; }

void print_real_loc(FILE *fout, struct RealLocation rl);
void two_reg_print(FILE *fout, char *op, struct RealLocation arg1,
                   struct RealLocation arg2) {
    int arg1_would_need = arg1.is_deref || arg1.type == TAGNAME;
    int arg2_would_need = arg2.is_deref || arg2.type == TAGNAME;

    if (!(arg1_would_need && arg2_would_need)) {
        fprintf(fout,
                "\t"
                "%s\t",
                op);
        print_real_loc(fout, arg1);
        fprintf(fout, ", ");
        print_real_loc(fout, arg2);
        fprintf(fout, "\n");
        return;
    }
    fprintf(fout, "\t"
                  "movl\t");
    print_real_loc(fout, arg1);
    fprintf(fout, ", ");
    struct RealLocation scratch = get_available_scratch();
    print_real_loc(fout, scratch);
    fprintf(fout, "\n");
    fprintf(fout,
            "\t"
            "%s\t",
            op);
    print_real_loc(fout, scratch);
    fprintf(fout, ", ");
    print_real_loc(fout, arg2);
    fprintf(fout, "\n");
    kill_reg(scratch.reg);
}

char *get_str() {
    static size_t str_counter = 0;
    int length = str_counter == 0 ? 1 : log10(str_counter) + 1;
    char *name = (char *)malloc(sizeof(char) * (6 + length));
    sprintf(name, "$.LOS%zu", str_counter++);
    return name;
}

struct RealLocation convert_to_real(struct Location l) {
    struct RealLocation rl;
    rl.is_deref = l.deref != 0;
    rl.deref_overflow = l.deref > 1;
    rl.deref_overflow = 0;
    rl.scale = 1;
    rl.disp = 0;
    rl.index_constant = 0;
    rl.index_is_reg = 0;
    switch (l.loc_type) {
    case REG: {
        if (l.reg == EMPTY_VREG) {
            fprintf(stderr, "UNRECHABLE EMPTY REG\n");
            exit(1);
        }
        int is_spill;
        rl.type = REGISTER;
        rl.reg = pick_register(l.reg, &is_spill);
        if (is_spill) {
            if (rl.is_deref) {
                rl.deref_overflow = 1;
            }
            rl.is_deref = 1;
            rl.disp = v_reg_counter.arr[l.reg].spill_offset;
        } else {
            fprintf(stderr, "Vreg %d was not spill\n", l.reg);
            exit(1);
        }
        break;
    }
    case VAR:
        // if it sa
        if (l.var->symbol_loc == GLOBAL || l.var->type == FUNCTION) {
            rl.type = TAGNAME;
            rl.tag.tag_name = l.var->name;
            rl.tag.is_owned = 0;
        } else {
            rl.type = REGISTER;
            rl.reg = EBP;
            rl.disp = l.var->offset;
            if (rl.is_deref) {
                rl.deref_overflow = 1;
            }
            rl.is_deref = 1;
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
        // fallthrough
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
    case ESP:
        reg_name[4] = 0;
        reg_name[3] = 'p';
        reg_name[2] = 's';
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
            fprintf(fout, "(,");
            if (rl.index_is_reg) {
                char reg_name[5];
                get_reg_name(rl.reg, reg_name);
                fprintf(fout, "%s", reg_name);
            }
            // else {
            // fprintf(fout, "%lld", rl.index_constant);
            // fprintf(fout, "");
            // }
            fprintf(fout, "%d)", rl.scale);
        }
        if (rl.tag.is_owned) {
            free(rl.tag.tag_name);
        }
        break;
    }
    case REGISTER: {
        char reg_name[5];
        get_reg_name(rl.reg, reg_name);
        if (rl.reg == SPILL) {
            fprintf(fout, "%lld(%s)", rl.disp, reg_name);
            break;
        }
        if (rl.is_deref) {
            fprintf(fout, "%lld(%s, ", rl.disp, reg_name);
            if (rl.index_is_reg) {
                get_reg_name(rl.index_reg, reg_name);
                fprintf(fout, "%s,", reg_name);
            }
            // else {
            //     fprintf(fout, "$%lld", rl.index_constant);
            // }
            fprintf(fout, "%d)", rl.scale);
            break;
        }
        fprintf(fout, "%s", reg_name);

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
                                   struct Type *func_type,
                                   struct BasicBlockArr *bba,
                                   size_t start_index);

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
                    fprintf(fout, "\t"
                                  ".section .rodata\n");
                fprintf(fout, ".LOS%zu:\n", str_counter++);
                fprintf(fout,
                        "\t"
                        ".string\t\"%s\"\n",
                        q.arg1.strlit.original_str);
            }
            if (q.arg2.loc_type == CONSTSTR) {
                if (first)
                    fprintf(fout, "\t"
                                  ".section .rodata\n");
                fprintf(fout, ".LOS%zu:\n", str_counter++);
                fprintf(fout,
                        "\t"
                        ".string\t\"%s\"\n",
                        q.arg2.strlit.original_str);
            }
        }
    }
}

void output_asm(char *output_file_name, struct BasicBlockArr *bba,
                struct SymbolTable *global_table) {

    FILE *fout = fopen(output_file_name, "w");

    initalize_data_and_bss_sections(fout, global_table);
    initalize_local_strs(fout, bba);
    fprintf(fout, "\t"
                  ".text\n");
    size_t i;
    struct SymbolTableNode *last_ref = NULL;

    for (i = 0; i < bba->len; ++i) {
        if (bba->arr[i].ref != NULL && last_ref != bba->arr[i].ref) {
            initalize_function_and_locals(fout, bba->arr[i].ref->name,
                                          bba->arr[i].ref->val.type, bba, i);
        }

        last_ref = bba->arr[i].ref;
        fprintf(fout, ".BB%zu:\n", i);
        output_basic_block(fout, &bba->arr[i]);
    }

    fclose(fout);
}

void initalize_function_and_locals(FILE *fout, char *name,
                                   struct Type *func_type,
                                   struct BasicBlockArr *bba,
                                   size_t start_index) {

    fprintf(fout,
            "\t"
            ".globl\t%s\n",
            name);
    fprintf(fout,
            "\t"
            ".type\t%s, @function\n",
            name);
    fprintf(fout, "%s:\n", name);
    fprintf(fout, "\t"
                  "pushl\t%%ebp\n");
    fprintf(fout, "\t"
                  "movl\t%%esp, %%ebp\n");
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
    i64 total_size = 4;
    for (i = func_type->extentions.func.arg_count;
         i < func_type->extentions.func.scope->len; ++i) {
        if (func_type->extentions.func.scope->nodearr[i]->type == FUNCTION) {
            continue;
        }
        func_type->extentions.func.scope->nodearr[i]->offset_marked = 1;
        u64 size = size_of_abstract(
            func_type->extentions.func.scope->nodearr[i]->val.type);
        u64 aligment = get_alignment(
            func_type->extentions.func.scope->nodearr[i]->val.type);
        // fprintf(stderr,
        //         "total_size = %lld\nvar %s has size %lld with aligment
        //         %lld\n", total_size,
        //         func_type->extentions.func.scope->nodearr[i]->name, size,
        //         aligment);
        if ((total_size + size) % aligment != 0) {
            // fprintf(stderr, "in if\n");
            total_size += aligment - size;
        }
        func_type->extentions.func.scope->nodearr[i]->offset =
            -(size + total_size);
        // fprintf(stderr, "total size = %lld, size = %lld\n", total_size,
        // size);
        total_size += size;
        // fprintf(stderr, "total size = %lld, size = %lld\n", total_size,
        // size);
    }
    // Find VReg Spilling
    VReg last = EMPTY_VREG;
    struct SymbolTableNode *last_ref = NULL;
    long long max_args = 0;
    for (i = start_index; i < bba->len; ++i) {
        if (bba->arr[i].ref != NULL && last_ref != bba->arr[i].ref) {
            if (i > start_index)
                break;
        }
        last_ref = bba->arr[i].ref;
        struct QuadListNode *current_q;
        for (current_q = bba->arr[i].head; current_q != NULL;
             current_q = current_q->next) {
            struct Quad q = current_q->quad;
            if (q.op == ARGBEGIN) {
                max_args = MAX(q.arg1.const_int, max_args);
            }
            if (q.eq.loc_type == REG &&
                v_reg_counter.arr[q.eq.reg].real_reg == NONE) {
                if (total_size % 4 != 0) {
                    total_size += 4 - (total_size % 4);
                }

                // fprintf(stderr, "spilling eq ");
                spill_register(q.eq.reg, -total_size);
                last = MAX(q.eq.reg, last);
                total_size += 4;
            }
            if (q.arg1.loc_type == REG &&
                v_reg_counter.arr[q.arg1.reg].real_reg == NONE) {
                if (total_size % 4 != 0) {
                    total_size += 4 - (total_size % 4);
                }
                // fprintf(stderr, "spilling arg1 ");
                spill_register(q.arg1.reg, -total_size);
                last = MAX(q.arg1.reg, last);
                total_size += 4;
            }
            if (q.arg2.loc_type == REG &&
                v_reg_counter.arr[q.arg2.reg].real_reg == NONE) {
                if (total_size % 4 != 0) {
                    total_size += 4 - (total_size % 4);
                }
                // fprintf(stderr, "spilling arg2 ");
                spill_register(q.arg2.reg, -total_size);
                last = MAX(q.arg2.reg, last);
                total_size += 4;
            }
        }
    }
    // fprintf(stderr, "last = %d\n", last);
    // for (i = 0; i < last; ++i) {
    // fprintf(stderr, "i %zu: is spill = %d\n", i,
    //         v_reg_counter.arr[i].real_reg == SPILL);
    // }
    total_size += max_args * 4;
    if (total_size % 16 != 0) {
        total_size += 16 - (total_size % 16);
    }
    if (total_size > 0)
        fprintf(fout,
                "\t"
                "subl\t$%lld, %%esp\n",
                total_size);
}

int location_eq(struct Location l1, struct Location l2) {
    if (l1.loc_type != l2.loc_type || l1.deref != l2.deref)
        return 0;
    switch (l1.loc_type) {
    case REG:
        return l1.reg == l2.reg;
    case VAR:
        return l1.var == l2.var;
    case CONSTINT:
        return 0;
    case CONSTFLOAT:
        return 0;
    case CONSTSTR:
        return 0;
    case BASICBLOCKNUM:
        return 0;
    }
    return 0;
}

struct RealLocation do_double_deref(FILE *fout, struct RealLocation rl) {
    fprintf(fout, "\t"
                  "movl\t");
    struct RealLocation reg = get_available_scratch();
    print_real_loc(fout, rl);
    fprintf(fout, ", ");
    print_real_loc(fout, reg);
    fprintf(fout, "\n");
    return reg;
}

void print_three_in_two(FILE *fout, char *op, struct Quad *q) {
    struct RealLocation rl1;
    struct RealLocation rl2;
    int in_else = 0;
    if (location_eq(q->eq, q->arg1)) {
        rl1 = convert_to_real(q->arg2);
        rl2 = convert_to_real(q->arg1);
    } else if (location_eq(q->eq, q->arg2)) {
        rl1 = convert_to_real(q->arg1);
        rl2 = convert_to_real(q->arg2);
    } else {
        // move one to a register
        rl1 = convert_to_real(q->arg1);
        rl2 = get_available_scratch();
        // print_real_loc(stderr, rl2);
        two_reg_print(fout, "movl", convert_to_real(q->arg2), rl2);
        // two_reg_print(stderr, "movl", convert_to_real(q->arg2), rl2);
        in_else = 1;
    }
    two_reg_print(fout, op, rl1, rl2);
    if (in_else) {
        struct RealLocation eq = convert_to_real(q->eq);
        if (eq.deref_overflow) {
            struct RealLocation reg = do_double_deref(fout, eq);
            int i;
            for (i = q->eq.deref; i > 1; --i) {
                struct RealLocation reg_old = reg;
                reg_old.is_deref = 1;
                two_reg_print(fout, "movl", reg_old, reg);
            }
            reg.is_deref = 1;
            two_reg_print(fout, "movl", rl2, reg);
            kill_reg(reg.reg);
        } else {
            two_reg_print(fout, "movl", rl2, eq);
        }
        kill_reg(rl2.reg);
    }
}

void output_quad(FILE *fout, struct Quad *q) {
    static size_t arg_arr_size = 0;
    static struct RealLocation *arg_arr = NULL;
    switch (q->op) {
    case LOAD: {
        struct RealLocation reg = get_available_scratch();
        two_reg_print(fout, "movl", convert_to_real(q->arg1), reg);
        int i;
        for (i = q->arg1.deref; i > 0; --i) {
            struct RealLocation reg_old = reg;
            reg_old.is_deref = 1;
            two_reg_print(fout, "movl", reg_old, reg);
        }
        reg.is_deref = 1;
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case STORE:
        fprintf(stderr, "SHOULD NOT BE USED");
        exit(1);
        break;
    case LEA: {
        struct RealLocation rl1 = convert_to_real(q->arg1);
        switch (rl1.type) {
        case TAGNAME:
            fprintf(fout, "\t"
                          "movl\t$");
            print_real_loc(fout, rl1);
            fprintf(fout, ", ");
            print_real_loc(fout, convert_to_real(q->eq));
            fprintf(fout, "\n");
            break;
        case REGISTER: {
            fprintf(fout, "\t"
                          "leal\t");
            print_real_loc(fout, convert_to_real(q->arg1));
            fprintf(fout, ", ");
            struct RealLocation reg = get_available_scratch();
            print_real_loc(fout, reg);
            fprintf(fout, "\n");
            two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
            kill_reg(reg.reg);
            break;
        }
        case CONST:
            fprintf(stderr, "UNRECHABLE\n");
            exit(1);
        }
        break;
    }
    case CVTFL:
        fprintf(stderr, "floats not supported\n");
        break;
    case CVTFQ:
        fprintf(stderr, "floats not supported\n");
        break;
    case CVTBS:
        // fallthrough
    case CVTWS:
        // fallthrough
    case CVTLS:
        // fallthrough
    case CVTQS:
        // fallthrough
    case CVTBU:
        // fallthrough
    case CVTWU:
        // fallthrough
    case CVTLU:
        // fallthrough
    case CVTQU:
        // fallthrough
    case MOV: {
        if (!q->eq.deref) {
            two_reg_print(fout, "movl", convert_to_real(q->arg1),
                          convert_to_real(q->eq));
            // print_three_in_two(fout, "movl", q);
            break;
        }
        struct RealLocation rl = convert_to_real(q->eq);
        fprintf(fout, "\t"
                      "movl\t");
        print_real_loc(fout, rl);
        fprintf(fout, ", ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        reg.is_deref = 1;
        two_reg_print(fout, "movl", convert_to_real(q->arg1), reg);
        kill_reg(reg.reg);
        break;
    }
    case ADD:
        print_three_in_two(fout, "addl", q);
        break;
    case SUB: {
        struct Quad real_q = *q;
        real_q.arg1 = q->arg2;
        real_q.arg2 = q->arg1;
        print_three_in_two(fout, "subl", &real_q);
        break;
    }
    case MUL:
        print_three_in_two(fout, "imull", q);
        break;
    case DIV: {
        struct RealLocation rl = get_available_scratch();
        kill_reg(rl.reg);
        rl.reg = EDX; // if i do reg allocation this is not safe
        if (reg_live_map[EDX - STARTREG] == 1) {
            fprintf(stderr, "something is funcky");
        }
        two_reg_print(fout, "movl", convert_to_real(q->arg1), rl);
        fprintf(fout, "\t"
                      "idiv\t");
        print_real_loc(fout, convert_to_real(q->arg2));
        rl.reg = EAX;
        if (reg_live_map[EAX - STARTREG] == 1) {
            fprintf(stderr, "something is funcky");
        }
        two_reg_print(fout, "movl", rl, convert_to_real(q->eq));
        break;
    }
    case FADD:
        // break;
        // fallthrough
    case FSUB:
        // fallthrough
    case FMUL:
        // fallthrough
    case FDIV:
        fprintf(stderr, "FLOATING POINT NOT SUPPORTED\n");
        exit(4);
    case MOD: {
        struct RealLocation rl = get_available_scratch();
        kill_reg(rl.reg);
        rl.reg = EDX; // if i do reg allocation this is not safe
        if (reg_live_map[EDX - STARTREG] == 1) {
            fprintf(stderr, "something is funcky");
        }
        two_reg_print(fout, "movl", convert_to_real(q->arg1), rl);
        fprintf(fout, "\t"
                      "idiv\t");
        print_real_loc(fout, convert_to_real(q->arg2));
        two_reg_print(fout, "movl", rl, convert_to_real(q->eq));
        break;
    }
    case BINOT: {
        struct RealLocation const_one = make_constant(0xFFFFFFFF);
        if (location_eq(q->arg1, q->eq)) {
            two_reg_print(fout, "xorl", const_one, convert_to_real(q->arg1));
            break;
        }
        struct RealLocation reg = get_available_scratch();
        two_reg_print(fout, "movl", convert_to_real(q->arg1), reg);
        two_reg_print(fout, "xorl", const_one, reg);
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case BIAND:
        print_three_in_two(fout, "andl", q);
        break;
    case BIOR:
        print_three_in_two(fout, "orl", q);
        break;
    case BIXOR:
        print_three_in_two(fout, "xorl", q);
        break;
    case BISHL:
        print_three_in_two(fout, "shll", q);
        break;
    case BISHR:
        print_three_in_two(fout, "shrl", q);
        break;
    case CMP:
        if (q->arg1.loc_type == CONSTINT && q->arg2.loc_type == CONSTINT) {
            struct RealLocation reg = get_available_scratch();
            two_reg_print(fout, "movl", convert_to_real(q->arg2), reg);
            two_reg_print(fout, "cmpl", convert_to_real(q->arg1), reg);
            kill_reg(reg.reg);
            break;
        }
        two_reg_print(fout, "cmpl", convert_to_real(q->arg2),
                      convert_to_real(q->arg1));
        break;
    case BR:
        fprintf(fout, "\t"
                      "jmp\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BREQ:
        fprintf(fout, "\t"
                      "je\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRNEQ:
        fprintf(fout, "\t"
                      "jne\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRLT:
        fprintf(fout, "\t"
                      "jl\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRLE:
        fprintf(fout, "\t"
                      "jle\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRGT:
        fprintf(fout, "\t"
                      "jg\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRGE:
        fprintf(fout, "\t"
                      "jge\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BREQU:
        fprintf(fout, "\t"
                      "je\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRNEQU:
        fprintf(fout, "\t"
                      "jne\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRLTU:
        fprintf(fout, "\t"
                      "jb\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRLEU:
        fprintf(fout, "\t"
                      "jbe\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRGTU:
        fprintf(fout, "\t"
                      "ja\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case BRGEU:
        fprintf(fout, "\t"
                      "jae\t");
        print_real_loc(fout, convert_to_real(q->arg1));
        fprintf(fout, "\n");
        break;
    case CCEQ: {
        fprintf(fout, "\t"
                      "sete\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCNEQ: {
        fprintf(fout, "\t"
                      "setne\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCLT: {
        fprintf(fout, "\t"
                      "setjl\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCLE: {
        fprintf(fout, "\t"
                      "setjle\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCGT: {
        fprintf(fout, "\t"
                      "setjg\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCGE: {
        fprintf(fout, "\t"
                      "setjge\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCEQU: {
        fprintf(fout, "\t"
                      "sete\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCNEQU: {
        fprintf(fout, "\t"
                      "setne\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCLTU: {
        fprintf(fout, "\t"
                      "setjb\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCLEU: {
        fprintf(fout, "\t"
                      "setjbe\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCGTU: {
        fprintf(fout, "\t"
                      "setja\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case CCGEU: {
        fprintf(fout, "\t"
                      "setjae\t%%al\n");
        fprintf(fout, "\t"
                      "movzbl\t%%al, ");
        struct RealLocation reg = get_available_scratch();
        print_real_loc(fout, reg);
        fprintf(fout, "\n");
        two_reg_print(fout, "movl", reg, convert_to_real(q->eq));
        kill_reg(reg.reg);
        break;
    }
    case ARGBEGIN: {
        arg_arr_size = q->arg1.const_int;
        break;
    }
    case ARG: {
        struct RealLocation esp;
        esp.type = REGISTER;
        esp.is_deref = 1;
        esp.reg = ESP;
        esp.scale = 1;
        esp.index_is_reg = 0;
        esp.index_constant = 0;
        esp.disp = q->arg1.const_int * 4;
        two_reg_print(fout, "movl", convert_to_real(q->arg2), esp);
        break;
    }
    case CALL: {

        // int i;
        // for (i = (int)arg_arr_size - 1; i >= 0; --i) {
        //     fprintf(fout,"\t" "pushl\t");
        //     print_real_loc(fout, arg_arr[i]);
        //     fprintf(fout, "\n");
        // }
        // for (i = 0; i < REGISTERCOUNT; ++i) {
        //     if (reg_live_map[i]) {
        //         fprintf(stderr,
        //                 "SHOULDN'T HAPPEN live reg before function call\n");
        //         exit(1);
        //     }
        // }
        fprintf(fout, "\t"
                      "call\t");
        struct RealLocation rl = convert_to_real(q->arg1);
        print_real_loc(fout, rl);
        fprintf(fout, "\n");
        // fprintf(fout,"\t" "addl\t$%lld, %%esp\n", last_arg_size);
        two_reg_print(fout, "movl", get_register(EAX), convert_to_real(q->eq));
        // arg_arr_size = 0;
        // free(arg_arr);
        // arg_arr = NULL;
        kill_reg(EAX);

        break;
    }
    case RET:
        if (!(q->arg1.loc_type == REG && q->arg1.reg == EMPTY_VREG)) {
            two_reg_print(fout, "movl", convert_to_real(q->arg1),
                          get_register(EAX));
            kill_reg(EAX);
        }
        fprintf(fout, "\t"
                      "leave\n");
        fprintf(fout, "\t"
                      "ret\n");
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
    u64 alignment = size_of_abstract(current_t);
    alignment = MAX(alignment, 4);
    return alignment;
}

void initalize_bss_var(FILE *fout, struct SymbolTableNode *node) {
    if (node->val.sc.sd == SD_EXTERNAL) {
        return;
    }
    if (node->val.sc.sl == SL_INTERNAL) {
        fprintf(fout, "\t"
                      ".local\n");
    }
    fprintf(fout,
            "\t"
            ".comm\t%s,%lld,%lld\n",
            node->name, size_of_abstract(node->val.type),
            get_alignment(node->val.type));
}

void initalize_data_var(FILE *fout, struct SymbolTableNode *node) {
    if (node->val.sc.sd == SD_EXTERNAL) {
        // UNRECHABLE
        return;
    }
    if (node->val.sc.sl != SL_INTERNAL) {
        fprintf(fout,
                "\t"
                ".globl\t%s\n",
                node->name);
    }
    if (node->val.type->type == T_POINTER) {
        // its a string
        fprintf(fout, "\t"
                      ".section\t.rodata\n");
        fprintf(fout, ".LC%lld:\n", unnamed_loc_val);
        fprintf(fout,
                "\t"
                ".string\t%s\n",
                node->val.initalizer->strlit.original_str);
    }
    fprintf(fout,
            "\t"
            ".type\t%s, @object\n",
            node->name);
    fprintf(fout, "\t"
                  ".data\n");
    u64 alignment = get_alignment(node->val.type);
    fprintf(fout,
            "\t"
            ".align %lld\n",
            alignment);
    u64 size = size_of_abstract(node->val.type);
    fprintf(fout,
            "\t"
            ".size %s, %lld\n",
            node->name, size);
    fprintf(fout, "%s:\n", node->name);
    if (node->val.type->type == T_POINTER) {
        fprintf(fout,
                "\t"
                ".long\t.LC%lld",
                unnamed_loc_val++);
        return;
    }
    if (node->val.type->type == T_CHAR) {
        fprintf(fout,
                "\t"
                ".byte\t%c\n",
                node->val.initalizer->constant.val.chr);
    }
    if (size == 2) {
        fprintf(fout,
                "\t"
                ".value\t%lld\n",
                node->val.initalizer->constant.type > TLONGLONG
                    ? node->val.initalizer->constant.val.u_int
                    : (i64)node->val.initalizer->constant.val.u_int);
    } else if (size == 4) {
        fprintf(fout,
                "\t"
                ".long\t%lld\n",
                node->val.initalizer->constant.type > TLONGLONG
                    ? node->val.initalizer->constant.val.u_int
                    : (i64)node->val.initalizer->constant.val.u_int);
    } else if (size == 8) {
        u32 first =
            (node->val.initalizer->constant.val.u_int >> 4) & 0x0000FFFF;
        u32 second = node->val.initalizer->constant.val.u_int & 0x0000FFFF;
        fprintf(fout,
                "\t"
                ".long\t%lld\n",
                node->val.initalizer->constant.type > TLONGLONG ? first
                                                                : (i64)first);
        fprintf(fout,
                "\t"
                ".long\t%lld\n",
                node->val.initalizer->constant.type > TLONGLONG ? second
                                                                : (i64)second);
    }
}

void initalize_data_and_bss_sections(FILE *fout,
                                     struct SymbolTable *global_table) {
    size_t i;
    for (i = 0; i < global_table->len; ++i) {
        if (global_table->nodearr[i]->type != FUNCTION &&
            global_table->nodearr[i]->type != TAG) {
            if (global_table->nodearr[i]->val.initalizer == NULL)
                initalize_bss_var(fout, global_table->nodearr[i]);
            else
                initalize_data_var(fout, global_table->nodearr[i]);
        }
    }
}
