#pragma once
#include "./token_codes.h"
YYLVALTYPE convert_to_str(char *character, int len);
YYLVALTYPE convert_to_char(char *character, int len);
YYLVALTYPE convert_to_float(char *number, int len, int base);
YYLVALTYPE convert_to_int(char *number, int len, int base);
