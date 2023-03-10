%code requires {
    #include "./parser/yylval_types.h"
    #include "./parser/ast.h"
    #include <stdlib.h>
    #include <stdio.h>
    int yylex();
    void yyerror(const char *s);
}
%union{
    YYlvalNumLit num;
    YYlvalStrLit str;
    int op;
    struct AstNodeListNode* arg_expression_list;
    AstNode * astnode;

}

%token IDENT
%token TYPEDEF_NAME
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

// types
%type <astnode> IDENT constant primary_expression expression postfix_expression unary_expression cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression logical_and_expression logical_or_expression conditional_expression assignment_expression

%type <op> unary_operator assignment_operator
%type <arg_expression_list> argument_expression_list
%type <num> NUMBER
%type <str> STRING
%type <num> CHARLIT


%start expression_list
//%start translation_unit
%%

// Notes:
// Temporary start for assignemnt one
// This causes a bunch of unused errors for the grammer
// I am ignoring this until the next assignemnt when I need to implement them
// The conflict that I have's default resolution is correct so I will not fix it
// Also Idents right now are very ugly but I don't want to fix them until I have to implement them fully
expression_list: expression {
                    print_AstNode($1, 0);
               }
              | expression_list ';' expression {
                    print_AstNode($3, 0);
              }
              ;

// 6.4.4
constant: NUMBER {$$ = make_AstNode(ASTNODE_CONSTANT); AstNode * n = $$; n->constant = $1;}
        | CHARLIT {$$ = make_AstNode(ASTNODE_CONSTANT); AstNode * n = $$; n->constant = $1;}
        ;

// 6.5.1
primary_expression: IDENT {
                  $$ = make_IdentNode($<str>1);
                  }// More work needed to figure out what its value is (if its an enum ... ) scoping and all that jazz
                  | constant
                  | STRING {
                    $$ = make_AstNode(ASTNODE_STRLIT);
                    AstNode * n = $$;
                    n->strlit = $1;
                  }
                  | '(' expression ')' {$$ = $2;}
                  | generic_selection {yyerror("Not Implemented"); exit(1);}
                  ;
// 6.5.1.1
generic_selection: _GENERIC '(' assignment_expression ',' generic_assoc_list ')' {yyerror("Not Implemented"); exit(1);};
generic_assoc_list: generic_association {yyerror("Not Implemented"); exit(1);}
                  | generic_assoc_list ',' generic_association {yyerror("Not Implemented"); exit(1);}
                  ;
generic_association: type_name ':' assignment_expression {yyerror("Not Implemented"); exit(1);}
                   | DEFAULT ':' assignment_expression {yyerror("Not Implemented"); exit(1);}
                   ;
// 6.5.2
postfix_expression: primary_expression
                  | postfix_expression '[' expression ']' {
                      $$ = make_unary_op('*', make_binary_op('+', $1, $3));
                  }
                  | postfix_expression '(' argument_expression_list ')'  {
                      $$ = make_func_call($1, $3);
                  }// Optional
                  | postfix_expression '('')' {
                      $$ = make_func_call($1, NULL);
                  }
                  | postfix_expression  '.' IDENT {
                      $$ = make_binary_op('.', $1, make_IdentNode($<str>3));
                  }
                  | postfix_expression INDSEL IDENT {
                    AstNode * deref = make_unary_op('*', $1);
                    AstNode * ident = make_IdentNode($<str>3);
                    $$ = make_binary_op('.', deref, ident);
                  }
                  | postfix_expression PLUSPLUS {
                      $$ = make_unary_op(PLUSPLUS, $1);
                  }
                  | postfix_expression  MINUSMINUS {
                      $$ = make_unary_op(MINUSMINUS, $1);
                  }
                  | '(' type_name ')' '{' initalizer_list '}' {
                    yyerror("UNIMPLEMNTED");
                    exit(1);
                }// TODO IMPLEMENT
                  | '(' type_name ')' '{' initalizer_list ',' '}' {
                    yyerror("UNIMPLEMNTED");
                    exit(1);
                }// TODO IMPLEMENT
                  ;

argument_expression_list: assignment_expression {
                    $$ = make_node_list_node($1);
                }
                | argument_expression_list ',' assignment_expression{
                    $$ = append_AstNodeListNode($1, $3);
                }
                ;
// 6.5.3
unary_expression: postfix_expression
                | PLUSPLUS unary_expression {
                    AstNode * node  = make_AstNode(ASTNODE_CONSTANT);
                    NVal n;
                    n.u_int = 1;
                    YYlvalNumLit nl;
                    nl.type = TINT;
                    nl.val = n;
                    node->constant = nl;
                    $$ = make_binary_op(PLUSEQ, $2, node);
                }
                | MINUSMINUS unary_expression {
                    AstNode * node  = make_AstNode(ASTNODE_CONSTANT);
                    NVal n;
                    n.u_int = 1;
                    YYlvalNumLit nl;
                    nl.type = TINT;
                    nl.val = n;
                    node->constant = nl;
                    $$ = make_binary_op(MINUSEQ, $2, node);
                }
                | unary_operator cast_expression {
                    $$ = make_unary_op($1, $2);
                }
                | SIZEOF unary_expression {
                    $$ = make_unary_op(SIZEOF, $2);
                }
                | SIZEOF '(' type_name ')' {
                    yyerror("UNIMPLEMNTED");
                    exit(1);
                }

                | _ALIGNOF '(' type_name ')' {
                    yyerror("UNIMPLEMNTED");
                    exit(1);
                }
                ;

unary_operator: '&' {$$ = '&';}
              | '*' {$$ = '*';}
              | '+' {$$ = '+';}
              | '-' {$$ = '-';}
              | '~' {$$ = '~';}
              | '!' {$$ = '!';}
              ;

// 6.5.4
cast_expression: unary_expression
               | '(' type_name ')' cast_expression{
                    yyerror("UNIMPLEMNTED");
                    exit(1);
                }
               ;

// 6.5.5
multiplicative_expression: cast_expression
                         | multiplicative_expression '*' cast_expression {
                            $$ = make_binary_op('*', $1, $3);
                         }
                         | multiplicative_expression '/' cast_expression{
                            $$ = make_binary_op('/', $1, $3);
                         }
                         | multiplicative_expression '%' cast_expression{
                            $$ = make_binary_op('%', $1, $3);
                         }
                         ;


// 6.5.6
additive_expression: multiplicative_expression
                   | additive_expression '+' multiplicative_expression {
                       $$ = make_binary_op('+', $1, $3);
                   }
                   | additive_expression '-' multiplicative_expression{
                       $$ = make_binary_op('-', $1, $3);
                   }
                   ;


// 6.5.7
shift_expression: additive_expression
                | shift_expression SHL additive_expression{
                       $$ = make_binary_op(SHL, $1, $3);
                   }
                | shift_expression SHR additive_expression{
                       $$ = make_binary_op(SHR, $1, $3);
                   }
                ;


// 6.5.8
relational_expression: shift_expression
                     | relational_expression '<' shift_expression{
                       $$ = make_binary_op('<', $1, $3);
                   }
                     | relational_expression '>' shift_expression{
                       $$ = make_binary_op('>', $1, $3);
                   }
                     | relational_expression LTEQ shift_expression{
                       $$ = make_binary_op(LTEQ, $1, $3);
                   }
                     | relational_expression GTEQ shift_expression{
                       $$ = make_binary_op(GTEQ, $1, $3);
                   }
                     ;

// 6.5.9
equality_expression: relational_expression
                   | equality_expression EQEQ relational_expression {
                       $$ = make_binary_op(EQEQ, $1, $3);
                   }
                   | equality_expression NOTEQ relational_expression{
                       $$ = make_binary_op(NOTEQ, $1, $3);
                   }
                   ;

// 6.5.10
and_expression: equality_expression
              | and_expression '&' equality_expression {
                   $$ = make_binary_op('&', $1, $3);
              }
              ;

// 6.5.11
exclusive_or_expression: and_expression
                       | exclusive_or_expression '^' and_expression{
                           $$ = make_binary_op('^', $1, $3);
                      }
                       ;

// 6.5.12
inclusive_or_expression: exclusive_or_expression
                       | inclusive_or_expression '|' exclusive_or_expression {
                           $$ = make_binary_op('|', $1, $3);
                       }
                       ;

// 6.5.13
logical_and_expression: inclusive_or_expression
                      | logical_and_expression LOGAND inclusive_or_expression {
                           $$ = make_binary_op(LOGAND, $1, $3);
                      }
                      ;

// 6.5.14
logical_or_expression: logical_and_expression
                     | logical_or_expression LOGOR logical_and_expression {
                           $$ = make_binary_op(LOGOR, $1, $3);
                     }
                     ;

// 6.5.15
conditional_expression: logical_or_expression
                      | logical_or_expression '?' expression ':' conditional_expression{
                        $$ = make_ternary_op($1, $3, $5);
                      }
                      ;


// 6.5.16
assignment_expression: conditional_expression
                     | unary_expression assignment_operator assignment_expression {
                        $$ = make_binary_op($2, $1, $3);
                     }
                     ;
assignment_operator: '=' {$$ = '=';}
                   | TIMESEQ {$$ = TIMESEQ;}
                   | DIVEQ {$$ = DIVEQ;}
                   | MODEQ {$$ = MODEQ;}
                   | PLUSEQ {$$ = PLUSEQ;}
                   | MINUSEQ {$$ = MINUSEQ;}
                   | SHLEQ {$$ = SHLEQ;}
                   | SHREQ {$$ = SHREQ;}
                   | ANDEQ {$$ = ANDEQ;}
                   | XOREQ {$$ = XOREQ;}
                   | OREQ {$$ = OREQ;}
                   ;

// 6.5.17
expression: assignment_expression {
            $$ = $1;
          }
          | expression ',' assignment_expression {
            $$ = make_binary_op(',', $1, $3);
          }
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
              | TYPEDEF_NAME
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

struct_declarator_list: struct_declarator
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
enumerator: IDENT
          | IDENT '=' constant_expression
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
/* typedef_name: IDENT ; */

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

