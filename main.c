#include "./parser.tab.h"
#include "lexer/lexer_util.h"
#include "parser.tab.h"
#include "parser/symbol_table.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FileInfo file_info;
FILE *yyin;
struct SymbolTable *symbol_table;
int main(int argc, char **argv) {
    symbol_table = initalize_table(10);
    if (argc == 1) {
        file_info.file_line_start = 0;
        file_info.real_line_start = 0;
        char *temp = "stdin";
        file_info.file_name = (char *)malloc(sizeof(char) * 6);
        strcpy(file_info.file_name, temp);
        return yyparse();
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
            // free(file_info.file_name);
        }
        file_info.file_name =
            (char *)malloc(sizeof(char) * strnlen(argv[i], PATH_MAX));
        strncpy(file_info.file_name, argv[i], PATH_MAX);
        yyin = temp;
        int ret = yyparse();
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}
