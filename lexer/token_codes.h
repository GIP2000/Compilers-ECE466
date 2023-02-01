#pragma once
typedef enum {
  TOKEOF = 0,
  IDENT = 257, /* This is where yacc will put it */
  CHARLIT,
  STRING,
  NUMBER,
  INDSEL,
  PLUSPLUS,
  MINUSMINUS,
  SHL,
  SHR,
  LTEQ,
  GTEQ,
  EQEQ,
  NOTEQ,
  LOGAND,
  LOGOR,
  ELLIPSIS,
  TIMESEQ,
  DIVEQ,
  MODEQ,
  PLUSEQ,
  MINUSEQ,
  SHLEQ,
  SHREQ,
  ANDEQ,
  OREQ,
  XOREQ,
  AUTO,
  BREAK,
  CASE,
  CHAR,
  CONST,
  CONTINUE,
  DEFAULT,
  DO,
  DOUBLE,
  ELSE,
  ENUM,
  EXTERN,
  FLOAT,
  FOR,
  GOTO,
  IF,
  INLINE,
  INT,
  LONG,
  REGISTER,
  RESTRICT,
  RETURN,
  SHORT,
  SIGNED,
  SIZEOF,
  STATIC,
  STRUCT,
  SWITCH,
  TYPEDEF,
  UNION,
  UNSIGNED,
  VOID,
  VOLATILE,
  WHILE,
  _ALIGNAS,
  _ALIGNOF,
  _ATOMIC,
  _BOOL,
  _COMPLEX,
  _GENERIC,
  _IMAGINARY,
  _NORETURN,
  _STATIC_ASSERT,
  _THREAD_LOCAL,
  LN,
} Tokens;
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
  double flt;
  unsigned char chr;
  char *str;
} YYNVal;

typedef struct {
  YYNVal value;
  ConstantTypes type;
} YYLVALTYPE;
