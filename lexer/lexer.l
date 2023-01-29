%option noyywrap
%{
#include "./lexer/token_codes.h"
    YYSTYPE yylval;
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
0[0-7]+[Uu]?[Ll]?[Ll]? {return NUMBER;}
[0-9]+[Uu]?[Ll]?[Ll]? {return NUMBER;}
0x[0-9A-Fa-f]+[Uu]?[Ll]?[Ll]? {return NUMBER;}
[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[FfLl]? {return NUMBER;}
[0-9]+([eE][+-]?[0-9]+)?[FfLl]? {return NUMBER;}
0x[0-9A-Fa-f]*\.[0-9A-Fa-f]+[Pp][+-]?[0-9][FfLl]? {return NUMBER;}
0x[0-9A-Fa-f]+[Pp][+-]?[0-9][FfLl]? {return NUMBER;}
[LUu]?'[^'\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?)' {return CHARLIT;}
(u8|u|U|L)?\"([^\"\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?))*\" {return STRING;}
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
            case CHARLIT: printf("CHARLIT\n");break;
            case STRING: printf("STRING\n");break;
            case NUMBER: printf("NUMBER\n");break;
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
