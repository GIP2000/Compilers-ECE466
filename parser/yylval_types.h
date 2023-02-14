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
    int nodetype;
    char *str;
    ConstantTypes type;
    int str_len;
} AstNodeStrLit;

typedef union {
    unsigned long long u_int;
    long double flt;
    unsigned char chr;
} NVal;

typedef struct {
    int nodetype;
    NVal val;
    ConstantTypes type;
} AstNodeNumLit;
