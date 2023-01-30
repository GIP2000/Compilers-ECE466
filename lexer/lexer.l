%option noyywrap
%option yylineno
%{
#include "./lexer/token_codes.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


YYLVALTYPE yylval;

YYLVALTYPE convert_to_str(char* character, int len) {
    int has_prefix = character[0] != '"';
    has_prefix = has_prefix + has_prefix*(character[1] == '8');
    printf("has_prefix: %d\n", has_prefix);
    int t_len = len - 2 - has_prefix + 1;
    printf("len: %d\n",t_len);
    char* str = (char* ) malloc(sizeof(char) * t_len); // TODO free this after use
    sscanf(character+has_prefix, "\"%s\"", str); // NOTE this is techincaly grabbing the last " but i remove it anyway
    str[t_len-1] = 0;
    NVALTYPE num;
    num.str = str;
    YYLVALTYPE r_val;
    r_val.value = num;
    r_val.type = Tu8;
    if (has_prefix == 1) {
        switch(character[0]) {
            case 'u': r_val.type = Tu; break;
            case 'U': r_val.type = TU; break;
            case 'L': r_val.type = TL; break;
            default: exit(1); fprintf(stderr, "Lexing error this should never happen");
        }
    }
    return r_val;
}

YYLVALTYPE convert_to_char(char* character, int len) {
    char prefix = character[0];
    int has_prefix = prefix != '\'';
    char val;
    sscanf(character+has_prefix, "'%c'", &val);
    NVALTYPE num;
    num.chr = val;
    YYLVALTYPE r_val;
    r_val.value = num;

    switch(prefix) {
        case 'L': r_val.type = TWCHAR; break;
        case 'u': r_val.type = TCHAR16;break;
        case 'U': r_val.type = TCHAR32;break;
        default: r_val.type = TUCHAR; break;
    }
    return r_val;
}

YYLVALTYPE convert_to_float(char* number, int len, int base) {
    char last = number[len-1];
    char * format_string = "%LF%*s";
    if (base == 16) {
        format_string = "0x%La%*s";
    }
    long double val;
    sscanf(number,format_string, &val);
    NVALTYPE num;
    num.flt = val;
    YYLVALTYPE r_val;
    r_val.value = num;
    r_val.type = TDOUBLE + (last == 'f') + (last == 'F') + 2*((last == 'L') + (last == 'l'));
    return r_val;
}


YYLVALTYPE convert_to_int(char* number, int len, int base) {
    unsigned long long val;
    char* format_string = "%llu%*s";
    // What is the base we are converting
    if(base == 16) {
        format_string = "0x%llx%*s";
    } else if (base == 8) {
        format_string = "0%llo%*s";
    } else if( base != 10) {
        fprintf(stderr, "ERROR convert_to_int called with invalid base (error should never reach end user)");
        exit(1);
    }

    // Get Suffix info
    int end = len < 3 ? len: 3;
    char * last_three = len < 3 ? number : number+len-3;
    int i;
    for(i = 0; i<end; ++i) {
        if(isdigit(last_three[i]))
            continue;
        break; // grabs the first suffix and puts it in i
    }
    int start_suffix = i;
    int is_unsigned = 0;
    if (i < end && (last_three[i] == 'U' ||last_three[i] == 'u')) { // checks if unsigned
        ++i;
        is_unsigned = 1;
    }

    for(; i<end; ++i) { // counts length
        if (!(last_three[i] == 'L' ||last_three[i] == 'l'))
            break;
    }

    // stores the number
    sscanf(number,format_string, &val);
    NVALTYPE num;
    num.u_int = val;
    YYLVALTYPE r_val;
    r_val.value = num;

    // gets the inital type
    r_val.type = TINT + (is_unsigned*3) + (i - start_suffix - is_unsigned);

    // checks to make sure it fits
    switch(r_val.type) {
        case TINT:  {
            if ((int)val == val) {
                break;
            }
            r_val.type = TLONG;
        }
        case TLONG: {
            if ((long)val == val) {
                break;
            }
            r_val.type = TLONGLONG;
        }
        case TLONGLONG: {
            if ((long long)val == val) {
                break;
            }
            // Ask what to do here
            break;
        }
        case TUINT: {
            if ((unsigned int)val == val) {
                break;
            }
            r_val.type = TULONG;
        }
        case TULONG: {
            if ((unsigned long)val == val) {
                break;
            }
            r_val.type = TULONGLONG;
        }
        case TULONGLONG: {
            if ((unsigned long long)val == val) {
                break;
            }
            // Ask what to do here
            break;
        }
        default: {
            fprintf(stderr,"Internal Compiler Error matched integer returns float type");
            exit(1);
        }
    }
    return r_val;
}

%}
%%
auto {return AUTO;}
break {return BREAK;}
case {return CASE;}
char {return CHAR;}
const {return CONST;}
continue {return CONTINUE;}
default {return DEFAULT;}
do {return DO;}
double {return DOUBLE;}
else {return ELSE;}
enum {return ENUM;}
extern {return EXTERN;}
float {return FLOAT;}
for {return FOR;}
goto {return GOTO;}
if {return IF;}
inline {return INLINE;}
int {return INT;}
long {return LONG;}
register {return REGISTER;}
restrict {return RESTRICT;}
return {return RETURN;}
short {return SHORT;}
signed {return SIGNED;}
sizeof {return SIZEOF;}
static {return STATIC;}
struct {return STRUCT;}
switch {return SWITCH;}
typedef {return TYPEDEF;}
union {return UNION;}
unsigned {return UNSIGNED;}
void {return VOID;}
volatile {return VOLATILE;}
while {return WHILE;}
_Alignas {return _ALIGNAS;}
_Alignof {return _ALIGNOF;}
_Atomic {return _ATOMIC;}
_Bool {return _BOOL;}
_Complex {return _COMPLEX;}
_Generic {return _GENERIC;}
_Imaginary {return _IMAGINARY;}
_Noreturn {return _NORETURN;}
_Static_assert {return _STATIC_ASSERT;}
_Thread_local {return _THREAD_LOCAL;}
__func__ {return __FUNC__;}
[A-Za-z_][A-Za-z_0-9]* {return IDENT;}
0[0-7]+[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 8);return NUMBER;}
[1-9][0-9]*[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 10);return NUMBER;}
0x[0-9A-Fa-f]+[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 16);return NUMBER;}
[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 10);return NUMBER;}
[0-9]+\.([eE][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 10);return NUMBER;}
0x[A-Fa-f0-9]*\.[A-Fa-f0-9]+([pP][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 16);return NUMBER;}
0x[A-Fa-f0-9]+\.([pP][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 16);return NUMBER;}
[LUu]?\'([^'\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?))' {yylval = convert_to_char(yytext, yyleng);return CHARLIT;}
(u8|u|U|L)?\"([^\"\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?))*\" {yylval = convert_to_str(yytext,yyleng); return STRING;}
[\[\]\(\)\{\}\.\&\*\+~!\/%<>\^\|\?:;=,#] {return yytext[0];}
-> {return INDSEL;}
\+\+ {return PLUSPLUS;}
-- {return MINUSMINUS;}
\<\< {return SHL;}
>> {return SHR;}
\<=  {return LTEQ;}
\>=  {return GTEQ;}
==  {return EQEQ;}
!=  {return NOTEQ;}
&&  {return LOGAND;}
\|\|  {return LOGOR;}
\.\.\.  {return ELLIPSIS;}
\*=  {return TIMESEQ;}
\/=  {return DIVEQ;}
\%=  {return MODEQ;}
\+=  {return PLUSEQ;}
-=  {return MINUSEQ;}
\<\<=  {return SHLEQ;}
\>\>=  {return SHREQ;}
&=  {return ANDEQ;}
\|=  {return OREQ;}
\^=  {return XOREQ;}
[\n ] {}
.                           {fprintf(stderr, "Error: Unkown Token (%s)\n", yytext);}
%%

int main() {
    int t;
    while(( t = yylex())) {
        switch(t) {
            case TOKEOF: printf("TOKEOF\n");break;
            case IDENT: printf("IDENT\n");break;
            case CHARLIT: printf("CHARLIT: %c %d\n", yylval.value.chr, yylval.type);break;
            case STRING: printf("STRING: %s %d\n", yylval.value.str, yylval.type);break;
            case NUMBER:{
                    if (yylval.type > 5) {
                        printf("NUMBER: %lf %d\n",yylval.value.flt, yylval.type);
                        break;
                    }
                    printf("NUMBER: %llu %d\n",yylval.value.u_int, yylval.type);
                    break;
                }

            case INDSEL: printf("INDSEL\n");break;
            case PLUSPLUS: printf("PLUSPLUS\n");break;
            case MINUSMINUS: printf("MINUSMINUS\n");break;
            case SHL: printf("SHL\n");break;
            case SHR: printf("SHR\n");break;
            case LTEQ: printf("LTEQ\n");break;
            case GTEQ: printf("GTEQ\n");break;
            case EQEQ: printf("EQEQ\n");break;
            case NOTEQ: printf("NOTEQ\n");break;
            case LOGAND: printf("LOGAND\n");break;
            case LOGOR: printf("LOGOR\n");break;
            case ELLIPSIS: printf("ELLIPSIS\n");break;
            case TIMESEQ: printf("TIMESEQ\n");break;
            case DIVEQ: printf("DIVEQ\n");break;
            case MODEQ: printf("MODEQ\n");break;
            case PLUSEQ: printf("PLUSEQ\n");break;
            case MINUSEQ: printf("MINUSEQ\n");break;
            case SHLEQ: printf("SHLEQ\n");break;
            case SHREQ: printf("SHREQ\n");break;
            case ANDEQ: printf("ANDEQ\n");break;
            case OREQ: printf("OREQ\n");break;
            case XOREQ: printf("XOREQ\n");break;
            case AUTO: printf("AUTO\n");break;
            case BREAK: printf("BREAK\n");break;
            case CASE: printf("CASE\n");break;
            case CHAR: printf("CHAR\n");break;
            case CONST: printf("CONST\n");break;
            case CONTINUE: printf("CONTINUE\n");break;
            case DEFAULT: printf("DEFAULT\n");break;
            case DO: printf("DO\n");break;
            case DOUBLE: printf("DOUBLE\n");break;
            case ELSE: printf("ELSE\n");break;
            case ENUM: printf("ENUM\n");break;
            case EXTERN: printf("EXTERN\n");break;
            case FLOAT: printf("FLOAT\n");break;
            case FOR: printf("FOR\n");break;
            case GOTO: printf("GOTO\n");break;
            case IF: printf("IF\n");break;
            case INLINE: printf("INLINE\n");break;
            case INT: printf("INT\n");break;
            case LONG: printf("LONG\n");break;
            case REGISTER: printf("REGISTER\n");break;
            case RESTRICT: printf("RESTRICT\n");break;
            case RETURN: printf("RETURN\n");break;
            case SHORT: printf("SHORT\n");break;
            case SIGNED: printf("SIGNED\n");break;
            case SIZEOF: printf("SIZEOF\n");break;
            case STATIC: printf("STATIC\n");break;
            case STRUCT: printf("STRUCT\n");break;
            case SWITCH: printf("SWITCH\n");break;
            case TYPEDEF: printf("TYPEDEF\n");break;
            case UNION: printf("UNION\n");break;
            case UNSIGNED: printf("UNSIGNED\n");break;
            case VOID: printf("VOID\n");break;
            case VOLATILE: printf("VOLATILE\n");break;
            case WHILE: printf("WHILE\n");break;
            case _ALIGNAS: printf("_ALIGNAS\n");break;
            case _ALIGNOF: printf("_ALIGNOF\n");break;
            case _ATOMIC: printf("_ATOMIC\n");break;
            case _BOOL: printf("_BOOL\n");break;
            case _COMPLEX: printf("_COMPLEX\n");break;
            case _GENERIC: printf("_GENERIC\n");break;
            case _IMAGINARY: printf("_IMAGINARY\n");break;
            case _NORETURN: printf("_NORETURN\n");break;
            case _STATIC_ASSERT: printf("_STATIC_ASSERT\n");break;
            case _THREAD_LOCAL: printf("_THREAD_LOCAL\n");break;
            default: printf("%c\n", t); break;
        }
    }
    printf("EOF\n");
    return 0;
}
