#include "./lexer_util.h"
#include "./token_codes.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *yyin;
int yylex(void);
YYLVALTYPE yylval;
FileInfo file_info;
int lex_file();

int main(int argc, char **argv) {
  if (argc <= 1) {
    file_info.file_line_start = 0;
    file_info.real_line_start = 0;
    char *temp = "stdin";
    file_info.file_name = (char *)malloc(sizeof(char) * 6);
    strcpy(file_info.file_name, temp);
    lex_file();
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
    lex_file();
  }
}
int lex_file() {
  int t;
  while ((t = yylex())) {
    switch (t) {
    case TOKEOF:
      printf("TOKEOF\n");
      break;
    case IDENT:
      printf("IDENT\n");
      break;
    case CHARLIT:
      printf("CHARLIT: %c %d\n", yylval.value.chr, yylval.type);
      break;
    case STRING:
      printf("STRING: %s %d %d\n", yylval.value.str, yylval.type,
             yylval.str_len);
      break;
    case NUMBER: {
      if (yylval.type > 5) {
        printf("NUMBER: %lf %d\n", yylval.value.flt, yylval.type);
        break;
      }
      printf("NUMBER: %llu %d\n", yylval.value.u_int, yylval.type);
      break;
    }

    case INDSEL:
      printf("INDSEL\n");
      break;
    case PLUSPLUS:
      printf("PLUSPLUS\n");
      break;
    case MINUSMINUS:
      printf("MINUSMINUS\n");
      break;
    case SHL:
      printf("SHL\n");
      break;
    case SHR:
      printf("SHR\n");
      break;
    case LTEQ:
      printf("LTEQ\n");
      break;
    case GTEQ:
      printf("GTEQ\n");
      break;
    case EQEQ:
      printf("EQEQ\n");
      break;
    case NOTEQ:
      printf("NOTEQ\n");
      break;
    case LOGAND:
      printf("LOGAND\n");
      break;
    case LOGOR:
      printf("LOGOR\n");
      break;
    case ELLIPSIS:
      printf("ELLIPSIS\n");
      break;
    case TIMESEQ:
      printf("TIMESEQ\n");
      break;
    case DIVEQ:
      printf("DIVEQ\n");
      break;
    case MODEQ:
      printf("MODEQ\n");
      break;
    case PLUSEQ:
      printf("PLUSEQ\n");
      break;
    case MINUSEQ:
      printf("MINUSEQ\n");
      break;
    case SHLEQ:
      printf("SHLEQ\n");
      break;
    case SHREQ:
      printf("SHREQ\n");
      break;
    case ANDEQ:
      printf("ANDEQ\n");
      break;
    case OREQ:
      printf("OREQ\n");
      break;
    case XOREQ:
      printf("XOREQ\n");
      break;
    case AUTO:
      printf("AUTO\n");
      break;
    case BREAK:
      printf("BREAK\n");
      break;
    case CASE:
      printf("CASE\n");
      break;
    case CHAR:
      printf("CHAR\n");
      break;
    case CONST:
      printf("CONST\n");
      break;
    case CONTINUE:
      printf("CONTINUE\n");
      break;
    case DEFAULT:
      printf("DEFAULT\n");
      break;
    case DO:
      printf("DO\n");
      break;
    case DOUBLE:
      printf("DOUBLE\n");
      break;
    case ELSE:
      printf("ELSE\n");
      break;
    case ENUM:
      printf("ENUM\n");
      break;
    case EXTERN:
      printf("EXTERN\n");
      break;
    case FLOAT:
      printf("FLOAT\n");
      break;
    case FOR:
      printf("FOR\n");
      break;
    case GOTO:
      printf("GOTO\n");
      break;
    case IF:
      printf("IF\n");
      break;
    case INLINE:
      printf("INLINE\n");
      break;
    case INT:
      printf("INT\n");
      break;
    case LONG:
      printf("LONG\n");
      break;
    case REGISTER:
      printf("REGISTER\n");
      break;
    case RESTRICT:
      printf("RESTRICT\n");
      break;
    case RETURN:
      printf("RETURN\n");
      break;
    case SHORT:
      printf("SHORT\n");
      break;
    case SIGNED:
      printf("SIGNED\n");
      break;
    case SIZEOF:
      printf("SIZEOF\n");
      break;
    case STATIC:
      printf("STATIC\n");
      break;
    case STRUCT:
      printf("STRUCT\n");
      break;
    case SWITCH:
      printf("SWITCH\n");
      break;
    case TYPEDEF:
      printf("TYPEDEF\n");
      break;
    case UNION:
      printf("UNION\n");
      break;
    case UNSIGNED:
      printf("UNSIGNED\n");
      break;
    case VOID:
      printf("VOID\n");
      break;
    case VOLATILE:
      printf("VOLATILE\n");
      break;
    case WHILE:
      printf("WHILE\n");
      break;
    case _ALIGNAS:
      printf("_ALIGNAS\n");
      break;
    case _ALIGNOF:
      printf("_ALIGNOF\n");
      break;
    case _ATOMIC:
      printf("_ATOMIC\n");
      break;
    case _BOOL:
      printf("_BOOL\n");
      break;
    case _COMPLEX:
      printf("_COMPLEX\n");
      break;
    case _GENERIC:
      printf("_GENERIC\n");
      break;
    case _IMAGINARY:
      printf("_IMAGINARY\n");
      break;
    case _NORETURN:
      printf("_NORETURN\n");
      break;
    case _STATIC_ASSERT:
      printf("_STATIC_ASSERT\n");
      break;
    case _THREAD_LOCAL:
      printf("_THREAD_LOCAL\n");
      break;
    case LN:
      printf("LNA -> cf = %s, ln = %d\n", file_info.file_name,
             file_info.file_line_start);
      break;
    default:
      printf("%c\n", t);
      break;
    }
  }
  return 0;
}
