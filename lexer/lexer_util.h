#pragma once
#include "../parser.tab.h"

YYSTYPE convert_to_str(char *character, int len);
YYSTYPE convert_to_char(char *character);
YYSTYPE convert_to_float(char *number, int len, int base);
YYSTYPE convert_to_int(char *number, int len, int base);
YYSTYPE convert_to_ident(char *number, int len);
void get_file_info(char *file_info_str, int length, FileInfo *file_info);
