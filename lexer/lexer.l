%option noyywrap
%option yylineno
%{
#include "./lexer/token_codes.h"
#include "./lexer/lexer_util.h"
YYLVALTYPE yylval;
FileInfo file_info;
%}
%%
#\ [0-9]+\ \".+\"(\ [0-9]+)* {file_info.real_line_start = yylineno;get_file_info(yytext, yyleng, &file_info); return LN;}
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
[A-Za-z_][A-Za-z_0-9]* {return IDENT;}
0[0-7]*[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 8);return NUMBER;}
[1-9][0-9]*[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 10);return NUMBER;}
0x[0-9A-Fa-f]+[Uu]?[Ll]?[Ll]? {yylval = convert_to_int(yytext, yyleng, 16);return NUMBER;}
[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 10);return NUMBER;}
[0-9]+\.([eE][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 10);return NUMBER;}
0x[A-Fa-f0-9]*\.[A-Fa-f0-9]+([pP][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 16);return NUMBER;}
0x[A-Fa-f0-9]+\.([pP][+-]?[0-9]+)?[FfLl]? {yylval = convert_to_float(yytext, yyleng, 16);return NUMBER;}
[LUu]?\'([^'\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?))' {yylval = convert_to_char(yytext, yyleng);return CHARLIT;}
(u8|u|U|L)?\"([^\"\\\n]|(\\(['"?\\abfnrtv]|[0-7][0-7]?[0-7]?))|(\\x[0-9A-Fa-f][0-9A-Fa-f]?))*\" {yylval = convert_to_str(yytext,yyleng); return STRING;}
[\[\]\(\)\{\}\.\&\*\+~!\/%<>\^\|\?:;=,] {return yytext[0];}
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
.                           {fprintf(stderr, "%s:%d: Error: Unkown Token (%s)\n", file_info.file_name,yylineno - file_info.real_line_start + file_info.file_line_start, yytext); exit(1);}
%%
