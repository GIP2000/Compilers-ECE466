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
    char *str;
    char *original_str;
    ConstantTypes type;
    int str_len;
} YYlvalStrLit;

typedef union {
    unsigned long long u_int;
    long double flt;
    unsigned char chr;
} NVal;

typedef struct {
    NVal val;
    ConstantTypes type;
} YYlvalNumLit;
