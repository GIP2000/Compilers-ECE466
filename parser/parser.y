%code requires {
    int yylex();
    void yyerror(const char *s);
    #include "./parser/yylval_types.h"
}
%union{
    struct AstNodeGeneric {int nodetype;} generic;
    AstNodeNumLit num;
    AstNodeStrLit str;
}
%token IDENT
%token CHARLIT
%token STRING
%token NUMBER
%token INDSEL
%token PLUSPLUS
%token MINUSMINUS
%token SHL
%token SHR
%token LTEQ
%token GTEQ
%token EQEQ
%token NOTEQ
%token LOGAND
%token LOGOR
%token ELLIPSIS
%token TIMESEQ
%token DIVEQ
%token MODEQ
%token PLUSEQ
%token MINUSEQ
%token SHLEQ
%token SHREQ
%token ANDEQ
%token OREQ
%token XOREQ
%token AUTO
%token BREAK
%token CASE
%token CHAR
%token CONST
%token CONTINUE
%token DEFAULT
%token DO
%token DOUBLE
%token ELSE
%token ENUM
%token EXTERN
%token FLOAT
%token FOR
%token GOTO
%token IF
%token INLINE
%token INT
%token LONG
%token REGISTER
%token RESTRICT
%token RETURN
%token SHORT
%token SIGNED
%token SIZEOF
%token STATIC
%token STRUCT
%token SWITCH
%token TYPEDEF
%token UNION
%token UNSIGNED
%token VOID
%token VOLATILE
%token WHILE
%token _ALIGNAS
%token _ALIGNOF
%token _ATOMIC
%token _BOOL
%token _COMPLEX
%token _GENERIC
%token _IMAGINARY
%token _NORETURN
%token _STATIC_ASSERT
%token _THREAD_LOCAL
%token LN



%%


start: translation_unit ;

/* start: expression_list; */
/* expression_list: expression */
/*                | expression_list ',' expression */
/*                ; */

// 6.4.4
constant: NUMBER
        | enumeration_constant
        | CHARLIT
        ;


// 6.4.4.3
enumeration_constant: IDENT;


// 6.5.1
primary_expression: IDENT
                  | constant
                  | STRING
                  | '(' expression ')'
                  | generic_selection
                  ;
// 6.5.1.1
generic_selection: _GENERIC '(' assignment_expression ',' generic_assoc_list ')';
generic_assoc_list: generic_association
                  | generic_assoc_list ',' generic_association
                  ;
generic_association: type_name ':' assignment_expression
                   | DEFAULT ':' assignment_expression
                   ;
// 6.5.2
postfix_expression: primary_expression
                  /* | postfix_expression '[' expression ']' */
                  | postfix_expression '(' argument_expression_list ')' // Optional
                  | postfix_expression '('')'
                  | postfix_expression  '.' IDENT
                  | postfix_expression  INDSEL IDENT
                  | postfix_expression  PLUSPLUS IDENT
                  | postfix_expression  MINUSMINUS IDENT
                  | '(' type_name ')' '{' initalizer_list '}'
                  | '(' type_name ')' '{' initalizer_list ',' '}'
                  ;

argument_expression_list: assignment_expression
                        | argument_expression_list ',' assignment_expression
                        ;
// 6.5.3
unary_expression: postfix_expression
                | PLUSPLUS unary_expression
                | MINUSMINUS unary_expression
                | unary_operator cast_expression
                | SIZEOF unary_expression
                | SIZEOF '(' type_name ')'
                | _ALIGNOF '(' type_name ')'
                ;

unary_operator: '&'
              | '*'
              | '+'
              | '-'
              | '~'
              | '!'
              ;

// 6.5.4
cast_expression: unary_expression
               | '(' type_name ')' cast_expression
               ;

// 6.5.5
multiplicative_expression: cast_expression
                         | multiplicative_expression '*' cast_expression
                         | multiplicative_expression '/' cast_expression
                         | multiplicative_expression '%' cast_expression
                         ;


// 6.5.6
additive_expression: multiplicative_expression
                   | additive_expression '+' multiplicative_expression
                   | additive_expression '-' multiplicative_expression
                   ;


// 6.5.7
shift_expression: additive_expression
                | shift_expression SHL additive_expression
                | shift_expression SHR additive_expression
                ;


// 6.5.8
relational_expression: shift_expression
                     | relational_expression '<' shift_expression
                     | relational_expression '>' shift_expression
                     | relational_expression LTEQ shift_expression
                     | relational_expression GTEQ shift_expression
                     ;

// 6.5.9
equality_expression: relational_expression
                   | equality_expression EQEQ relational_expression
                   | equality_expression NOTEQ relational_expression
                   ;

// 6.5.10
and_expression: equality_expression
              | and_expression '&' equality_expression
              ;

// 6.5.11
exclusive_or_expression: and_expression
                       | exclusive_or_expression '^' and_expression
                       ;

// 6.5.12
inclusive_or_expression: exclusive_or_expression
                       | inclusive_or_expression '|' exclusive_or_expression
                       ;

// 6.5.13
logical_and_expression: inclusive_or_expression
                      | logical_and_expression LOGAND inclusive_or_expression
                      ;

// 6.5.14
logical_or_expression: logical_and_expression
                     | logical_or_expression LOGOR logical_and_expression
                     ;

// 6.5.15
conditional_expression: logical_or_expression
                      | logical_or_expression '?' expression ':' conditional_expression
                      ;


// 6.5.16
assignment_expression: conditional_expression
                     | unary_expression assignment_operator assignment_expression
                     ;
assignment_operator: '='
                   | TIMESEQ
                   | DIVEQ
                   | MODEQ
                   | PLUSEQ
                   | MINUSEQ
                   | SHLEQ
                   | SHREQ
                   | ANDEQ
                   | XOREQ
                   | OREQ
                   ;

// 6.5.17
expression: assignment_expression
          | expression ',' assignment_expression
          ;

// 6.6
constant_expression: conditional_expression ;

// 6.7
declaration: declaration_specifiers init_declarator_list ';' // Optional
           | declaration_specifiers ';'
           | static_assert_decleration
           ;

declaration_specifiers: storage_class_specifier declaration_specifiers // Optional
                      | storage_class_specifier
                      | type_specifier declaration_specifiers // Optional
                      | type_specifier
                      | type_qualifier declaration_specifiers // Optional
                      | type_qualifier
                      | function_specifier declaration_specifiers // Optional
                      | function_specifier
                      | alignment_specifier declaration_specifiers // Optional
                      | alignment_specifier
                      ;

init_declarator_list: init_declarator
                    | init_declarator_list ',' init_declarator
                    ;

init_declarator: declarator
               | declarator '=' initalizer
               ;
// 6.7.1
storage_class_specifier: TYPEDEF
                       | EXTERN
                       | STATIC
                       | _THREAD_LOCAL
                       | AUTO
                       | REGISTER
                       ;

// 6.7.2
type_specifier: VOID
              | CHAR
              | SHORT
              | INT
              | LONG
              | FLOAT
              | DOUBLE
              | SIGNED
              | UNSIGNED
              | _BOOL
              | _COMPLEX
              | atomic_type_specifier
              | struct_or_union_specifier
              | enum_specifier
              | typedef_name
              ;

// 6.7.2.1
struct_or_union_specifier: struct_or_union IDENT '{' struct_declaration_list '}' // Optional
                         | struct_or_union '{' struct_declaration_list '}'
                         | struct_or_union IDENT
                         ;

struct_or_union: STRUCT
               | UNION
               ;

struct_declaration_list: struct_declaration
                       | struct_declaration_list struct_declaration
                       ;
struct_declaration: specifier_qualifer_list struct_declarator_list ';' // Optional
                  | specifier_qualifer_list ';'
                  | static_assert_decleration
                  ;

specifier_qualifer_list: type_specifier specifier_qualifer_list // Optional
                       | type_specifier
                       | type_qualifier specifier_qualifer_list // Optional
                       | type_qualifier // Optional
                       ;

struct_declarator_list: struct_declarator_list
                      | struct_declarator_list ',' struct_declarator
                      ;

struct_declarator: declarator
                 | declarator ':' constant_expression // Optional
                 | ':' constant_expression // Optional
                 ;

// 6.7.2.2
enum_specifier: ENUM IDENT '{' enumerator_list '}' // Optional
              | ENUM  '{' enumerator_list '}'
              | ENUM IDENT '{' enumerator_list ',' '}' // Optional
              | ENUM  '{' enumerator_list ',' '}' // Optional
              | ENUM IDENT
              ;

enumerator_list: enumerator
               | enumerator_list ',' enumerator
               ;
enumerator: enumeration_constant
          | enumeration_constant '=' constant_expression
          ;

// 6.7.2.3 Come back to tags didn't see any grammer things

// 6.7.2.4
atomic_type_specifier: _ATOMIC '(' type_name ')';

// 6.7.3
type_qualifier: CONST
              | RESTRICT
              | VOLATILE
              | _ATOMIC
              ;

// 6.7.4
function_specifier: INLINE
                  | _NORETURN
                  ;

// 6.7.5
alignment_specifier: _ALIGNAS '(' type_name ')'
                   | _ALIGNAS '(' constant_expression ')'
                   ;

// 6.7.6
declarator: pointer direct_declarator
          | direct_declarator
          ;

direct_declarator: IDENT
                 | '(' declarator ')'
                 | direct_declarator '[' type_qualifier_list assignment_expression ']' // Double Optional
                 | direct_declarator '['type_qualifier_list ']'
                 | direct_declarator '[' assignment_expression ']'
                 | direct_declarator '[' ']'
                 | direct_declarator '[' STATIC type_qualifier_list assignment_expression']' // Optional
                 | direct_declarator '[' STATIC assignment_expression']'
                 | direct_declarator '[' type_qualifier_list STATIC assignment_expression']'
                 | direct_declarator '[' type_qualifier_list '*' ']'  // Optional
                 | direct_declarator '[' '*' ']'
                 | direct_declarator '(' parameter_type_list ')'
                 | direct_declarator '(' identifier_list ')'  // Optional
                 | direct_declarator '(' ')'
                 ;

pointer: '*' type_qualifier_list // Optional
       | '*'
       | '*' type_qualifier_list pointer // Optional
       | '*' pointer
       ;

type_qualifier_list: type_qualifier
                   | type_qualifier_list type_qualifier
                   ;

parameter_type_list: parameter_list
                   | parameter_list ',' ELLIPSIS
                   ;

parameter_list: parameter_declaration
              | parameter_list ',' parameter_declaration
              ;

parameter_declaration: declaration_specifiers declarator
                     | declaration_specifiers abstract_declarator // Optional
                     | declaration_specifiers
                     ;

identifier_list: IDENT
               | identifier_list ',' IDENT
               ;

// 6.7.7
type_name: specifier_qualifer_list abstract_declarator // Optional
         | specifier_qualifer_list
         ;

abstract_declarator: pointer
                   | pointer direct_abstract_declarator // Optional
                   | direct_abstract_declarator
                   ;

direct_abstract_declarator: '(' abstract_declarator ')'
                          | direct_abstract_declarator '[' type_qualifier_list assignment_expression ']' // Triple Optional
                          | '[' ']'
                          | '[' type_qualifier_list  ']'
                          | '[' assignment_expression ']'
                          | direct_abstract_declarator '[' ']'
                          | direct_abstract_declarator '[' type_qualifier_list  ']'
                          | direct_abstract_declarator '['  assignment_expression ']'
                          |  '[' type_qualifier_list assignment_expression ']'
                          | direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']' // Double Optional
                          | '[' STATIC  assignment_expression ']'
                          | direct_abstract_declarator '[' STATIC assignment_expression ']'
                          |  '[' STATIC type_qualifier_list assignment_expression ']'
                          | direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']' // Optional
                          |  '[' type_qualifier_list STATIC assignment_expression ']'
                          | direct_abstract_declarator '[' '*' ']' // Optional
                          |  '[' '*' ']'
                          | direct_abstract_declarator '(' parameter_type_list ')'  // Double Optional
                          | direct_abstract_declarator '(' ')'
                          |  '(' parameter_type_list ')'
                          | '(' ')'
                          ;

// 6.7.8
typedef_name: IDENT ;

// 6.7.9
initalizer:  assignment_expression
          | '{' initalizer_list '}'
          | '{' initalizer_list ',' '}'
          ;

initalizer_list: designation initalizer // Optional
               | initalizer
               | initalizer_list ',' designation initalizer // Optional
               | initalizer_list ',' initalizer
               ;

designation: designator_list '=' ;

designator_list: designator
               | designator_list designator
               ;

designator: '[' constant_expression ']'
          | '.' IDENT
          ;

// 6.7.10
static_assert_decleration: _STATIC_ASSERT '(' constant_expression ',' STRING ')' ';' ;

// 6.8
statment: labeled_statment
        | compound_statment
        | expression_statment
        | selection_statment
        | iteration_statment
        | jump_statment
        ;

// 6.8.1
labeled_statment: IDENT ':' statment
                | CASE constant_expression ':' statment
                | DEFAULT ':' statment
                ;

// 6.8.2
compound_statment: '{' block_item_list '}' // Optional
                 | '{' '}'
                 ;

block_item_list: block_item
               | block_item_list block_item
               ;

block_item: declaration
          | statment
          ;

// 6.8.3
expression_statment: expression ';'
                   | ';'
                   ;

// 6.8.4
selection_statment: IF '(' expression ')' statment
                  | IF '(' expression ')' ELSE statment
                  | SWITCH '(' expression ')' statment
                  ;

iteration_statment: WHILE '(' expression ')' statment
                  | DO statment WHILE '(' expression ')' ';'
                  | FOR '(' expression ';' expression ';' expression ')' statment // Triple Optional
                  | FOR '(' ';' ';' ')' statment
                  | FOR '(' expression ';'  ';'  ')' statment
                  | FOR '('  ';' expression ';'  ')' statment
                  | FOR '('  ';'  ';' expression ')' statment
                  | FOR '(' expression ';'  ';' expression ')' statment
                  | FOR '(' expression ';' expression ';'  ')' statment
                  | FOR '('  ';' expression ';' expression ')' statment
                  | FOR '(' declaration expression ';' expression ')' statment // Double Optional
                  | FOR '(' declaration  ';'  ')' statment
                  | FOR '(' declaration expression ';'  ')' statment
                  | FOR '(' declaration  ';' expression ')' statment
                  ;

// 6.8.6
jump_statment: GOTO IDENT ';'
             | CONTINUE ';'
             | BREAK ';'
             | RETURN expression ';' // Optional
             | RETURN ';'
             ;

// 6.9
translation_unit: external_declaration
                | translation_unit external_declaration
                ;

external_declaration: function_definition
                    | declaration
                    ;
// 6.9.1
function_definition: declaration_specifiers declarator declaration_list compound_statment // Optional
                   | declaration_specifiers declarator compound_statment
                   ;

declaration_list: declaration
                | declaration_list declaration
                ;

