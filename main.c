#include "./lexer/file_info.h"
#include "./parser.tab.h"
#include "lexer/lexer_util.h"
#include "parser.tab.h"
#include "parser/ast.h"
#include "parser/symbol_table.h"
#include "quads/ast_parser.h"
#include "quads/quad.h"
#include "target_code/target.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #ifdef YYDEBUG
// extern int yydebug;
// #endif

extern FileInfo file_info;
FILE *yyin;
struct SymbolTable *symbol_table;

int main(int argc, char **argv) {
    // #ifdef YYDEBUG
    //     yydebug = 1;
    // #endif
    symbol_table = initalize_table(10);
    if (argc == 1) {
        file_info.file_line_start = 0;
        file_info.real_line_start = 0;
        char *temp = "stdin";
        file_info.file_name = (char *)malloc(sizeof(char) * 6);
        strcpy(file_info.file_name, temp);
        int result = yyparse();
        if (result != 0) {
            return result;
        }
        print_st(symbol_table);
        struct BasicBlockArr arr = build_bba_from_st(symbol_table);
        print_bba(&arr);
        output_asm("a.S", &arr, symbol_table);
        return 0;
    }
    int i;
    for (i = 1; i < argc; ++i) {
        FILE *temp = fopen(argv[i], "r");
        if (temp == NULL) {
            fprintf(stderr, "Error opening file %s", argv[i]);
            return 1;
        }
        file_info.file_line_start = 0;
        if (file_info.file_name != NULL) {
            free(file_info.file_name);
        }
        file_info.file_name =
            (char *)malloc(sizeof(char) * strnlen(argv[i], PATH_MAX));
        strncpy(file_info.file_name, argv[i], PATH_MAX);
        yyin = temp;
        int ret = yyparse();
        if (ret != 0) {
            return ret;
        }
        print_st(symbol_table);
        // pop_global_table();
        struct BasicBlockArr arr = build_bba_from_st(symbol_table);
        print_bba(&arr);
        if (argc == 2) {
            output_asm("a.S", &arr, symbol_table);
        } else {
            char output_file[5];
            sprintf(output_file, "a%d.S", i);
            output_asm(output_file, &arr, symbol_table);
        }
        symbol_table = initalize_table(10);
    }
    return 0;
}
