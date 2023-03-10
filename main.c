#include "./parser.tab.h"
#include "lexer/lexer_util.h"
#include "parser.tab.h"
#include "parser/ast.h"
#include "parser/symbol_table.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FileInfo file_info;
FILE *yyin;
struct SymbolTable *symbol_table;

void print_output() {
    size_t i;
    for (i = 0; i < symbol_table->len; ++i) {
        if (symbol_table->nodearr[i].type != 0)
            continue;
        printf("name: %s type ", symbol_table->nodearr[i].name);
        print_type(symbol_table->nodearr[i].val.type);
        if (symbol_table->nodearr[i].val.type->type == T_FUNC) {
            printf("{\n");
            print_AstNode(
                symbol_table->nodearr[i].val.type->extentions.func.statment, 0);
            printf("}");
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
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
        print_output();
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
        print_output();
    }
    return 0;
}
