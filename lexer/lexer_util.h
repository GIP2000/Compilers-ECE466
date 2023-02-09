#pragma once
typedef enum {
    TINT,
    TLONG,
    TLONGLONG,
    TUINT,
    TULONG,
    TULONGLONG,
    TDOUBLE,
    TFLOAT,
    TLONGDOUBLE,
    TUCHAR,
    TWCHAR,
    TCHAR16,
    TCHAR32,
    Tu8,
    Tu,
    TU,
    TL,
} ConstantTypes;

typedef struct {
    char *file_name;
    int file_line_start;
    int real_line_start;
} FileInfo;

typedef union {
    unsigned long long u_int;
    long double flt;
    unsigned char chr;
    char *str;
} YYNVal;

typedef struct {
    YYNVal value;
    ConstantTypes type;
    int str_len;
} YYLVALTYPE;
YYLVALTYPE convert_to_str(char *character, int len);
YYLVALTYPE convert_to_char(char *character, int len);
YYLVALTYPE convert_to_float(char *number, int len, int base);
YYLVALTYPE convert_to_int(char *number, int len, int base);
void get_file_info(char *file_info_str, int length, FileInfo *file_info);
