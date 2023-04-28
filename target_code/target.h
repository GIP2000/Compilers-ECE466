#pragma once
#include "../parser/symbol_table.h"
#include "../quads/quad.h"

void output_asm(char *output_file_name, struct BasicBlockArr *bba,
                struct SymbolTable *global_table);
