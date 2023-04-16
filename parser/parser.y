%code requires {
    #include "./parser/yylval_types.h"
    #include "./parser/ast.h"
    #include "./parser/symbol_table.h"
    #include "./parser/types.h"
    #include <stdlib.h>
    #include <stdio.h>
    int yylex();
    void yyerror(const char *s);
    extern struct SymbolTable * symbol_table;
}
%union{
    YYlvalNumLit num;
    YYlvalStrLit str;
    int op;
    struct AstNodeListNode* arg_expression_list;
    AstNode * astnode;
    struct EffectiveStorageClass storage_class;
    struct SymbolTableNode current_symbol;
    struct Type * type;
    enum TypeQualifier type_qualifier;
    enum FunctionSpecifier function_specifier;
    int flag;
    struct StNodeTablePair st_node_pair;
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
%type <astnode> constant primary_expression expression postfix_expression unary_expression cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression logical_and_expression logical_or_expression conditional_expression assignment_expression init_declarator_list initalizer parameter_list declaration statment compound_statment block_item_list block_item struct_declaration_list struct_declaration function_compount_statment selection_statment iteration_statment labeled_statment jump_statment expression_statment constant_expression

%type <arg_expression_list> argument_expression_list
%type <storage_class> storage_class_specifier
%type <current_symbol> declaration_specifiers init_declarator struct_declarator_list
%type <st_node_pair> direct_abstract_declarator direct_declarator abstract_declarator declarator parameter_declaration struct_declarator
%type <type> type_specifier pointer struct_or_union_specifier specifier_qualifer_list
%type <type_qualifier> type_qualifier type_qualifier_list
%type <function_specifier> function_specifier
%type <str> IDENT
%type <flag> parameter_type_list struct_or_union



%type <op> unary_operator assignment_operator
%type <num> NUMBER
%type <str> STRING
%type <num> CHARLIT


%start translation_unit
%%

// Notes:
// Temporary start for assignemnt one
// This causes a bunch of unused errors for the grammer
// I am ignoring this until the next assignemnt when I need to implement them
// The conflict that I have's default resolution is correct so I will not fix it
// Also Idents right now are very ugly but I don't want to fix them until I have to implement them fully

// 6.4.4
constant: NUMBER {$$ = make_ConstantType($1);}
        | CHARLIT {$$ = make_ConstantType($1);}
        ;

// 6.5.1
primary_expression: IDENT {
                  /* $$ = make_IdentNode($<str>1); */
                  $$ = make_IdentNode($1);
                  }// More work needed to figure out what its value is (if its an enum ... ) scoping and all that jazz
                  | constant
                  | STRING {
                    $$ = make_StringType($1);
                    /* $$ = make_AstNode(ASTNODE_STRLIT); */
                    /* AstNode * n = $$; */
                    /* n->strlit = $1; */
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
                      if($1->value_type->type != T_STRUCT || $1->value_type->type == T_UNION) {
                        yyerror("Invalid . operator must be used with a struct or union");
                        exit(2);
                      }
                      struct SymbolTable * old = symbol_table;
                      symbol_table = $1->value_type->extentions.st_un.mem;
                      $$ = make_binary_op('.', $1, make_IdentNode($3));
                      symbol_table = old;
                      /* $$ = make_binary_op('.', $1, make_IdentNode($<str>3)); */
                  }
                  | postfix_expression INDSEL IDENT {
                      if($1->value_type->type != T_POINTER &&
                          ($1->value_type->extentions.next_type.next->type == T_STRUCT
                            || $1->value_type->extentions.next_type.next->type == T_UNION )) {
                        yyerror("Invalid . operator must be used with a struct or union");
                        exit(2);
                      }
                    struct SymbolTable * old = symbol_table;
                    symbol_table = $1->value_type->extentions.next_type.next->extentions.st_un.mem;
                    $$ = make_binary_op(INDSEL, $1, make_IdentNode($3));
                    symbol_table = old;
                    /* AstNode * deref = make_unary_op('*', $1); */
                    /* AstNode * ident = make_IdentNode($3); */
                    /* AstNode * ident = make_IdentNode($<str>3); */
                    /* $$ = make_binary_op('.', deref, ident); */
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
declaration: declaration_specifiers init_declarator_list ';'  {$$ = $2;}// Optional
           | declaration_specifiers ';' {$$ = NULL;}
           | static_assert_decleration {fprintf(stderr, "UNIMPLEMNTED"); exit(1);}
           ;

declaration_specifiers: storage_class_specifier declaration_specifiers {
                          $2.val.sc = $1;
                          $$ = $2;
                      }// Optional
                      | storage_class_specifier {
                       $$ = make_st_node(NULL, 0, 0, $1, NULL, NULL);
                      }
                      | type_specifier declaration_specifiers {
                        $1->extentions.next_type.next = $2.val.type;
                        $2.val.type = $1;
                        $$ = $2;
                      }// Optional
                      | type_specifier {
                        $$ = make_st_node(NULL, 0, 0, get_default_sc(), $1, NULL);
                      }
                      | type_qualifier declaration_specifiers {
                        $2.val.type->qualifier_bit_mask |= $1;
                        $$ = $2;
                      }// Optional
                      | type_qualifier {
                        struct Type * t = make_default_type(0);
                        t->qualifier_bit_mask = $1;
                        $$ = make_st_node(NULL, 0, 0, get_default_sc(), t, NULL);
                      }
                      | function_specifier declaration_specifiers  {
                        $2.val.type->extentions.func.function_spec_bit_mask |= $1;
                        $$ = $2;
                      }// Optional
                      | function_specifier {
                        struct Type * t = make_default_type(T_FUNC);
                        t->extentions.func.function_spec_bit_mask = $1;
                        $$ = make_st_node(NULL, 0, 0, get_default_sc(), t, NULL);
                      }
                      | alignment_specifier declaration_specifiers {
                          yyerror("UNIMPLEMNTED");
                          exit(1);
                      }// Optional
                      | alignment_specifier{
                          yyerror("UNIMPLEMNTED");
                          exit(1);
                      }
                      ;

init_declarator_list: init_declarator {
                    /* fprintf(stderr, "I got here\n"); */
                    /* if (symbol_table->st_type == PROTOTYPE) { */
                    /*     shallow_pop_table(); */
                    /* } */
                    struct Type *t = add_to_end_and_reverse($1.val.type, $<current_symbol>0.val.type);
                        struct SymbolTableNode n = make_st_node($1.name, $1.namespc, $1.type, $<current_symbol>0.val.sc, t,$1.val.initalizer);
                        /* // TODO fill in IDENT TYPE */
                        if(!enter_in_namespace(n, ORD)) { //TODO ord is Temporary
                           yyerror("Varaible redefinition");
                           exit(2);
                        };

                        AstNode * decl = make_Declaration(symbol_table->nodearr[symbol_table->len - 1]);
                        $$ = make_StatementList(decl);
                    }
                    | init_declarator_list ',' init_declarator {
                        /* fprintf(stderr, "I got here 2 wd\n"); */
                        /* if (symbol_table->st_type == PROTOTYPE) { */
                        /*     shallow_pop_table(); */
                        /* } */
                        struct SymbolTableNode *n = $1->statments.head->node->declaration.symbol;
                        struct Type * t_to_add;
                        for(t_to_add = n->val.type; t_to_add != NULL && (t_to_add->type == T_POINTER || t_to_add->type == T_ARR); t_to_add = t_to_add->extentions.next_type.next) {}
                        struct Type * t = add_to_end_and_reverse($3.val.type, t_to_add);
                        struct SymbolTableNode node = make_st_node($3.name, $3.namespc, $3.type, n->val.sc, t,$3.val.initalizer);
                        if(!enter_in_namespace(node, ORD)) { //TODO ord is Temporary
                           yyerror("Varaible redefinition");
                           exit(2);
                        };

                        AstNode * decl = make_Declaration(symbol_table->nodearr[symbol_table->len - 1]);
                        append_StatmentList(&$1->statments, decl);
                        $$ = $1;
                    }
                    ;

init_declarator: declarator {$$ = $1.node;}
               | declarator '=' initalizer {
                    $1.node.val.initalizer = $3;
                    $$ = $1.node;
               }
               ;
// 6.7.1
storage_class_specifier: TYPEDEF {
                        yyerror("UNIMPLEMNTED");
                        exit(1);
                       }
                       | EXTERN {$$ = make_eff_storage_class(S_EXTERN);}
                       | STATIC {$$ = make_eff_storage_class(S_STATIC);}
                       | _THREAD_LOCAL {
                            yyerror("UNIMPLEMNTED");
                            exit(1);
                       }
                       | AUTO {
                           if (symbol_table->st_type == GLOBAL) {
                               yyerror("keyword auto not allowed in Global Scope");
                               exit(2);
                           }
                           $$ = make_eff_storage_class(S_AUTO);
                       }
                       | REGISTER {$$ = make_eff_storage_class(S_REG);}
                       ;

// 6.7.2
type_specifier: VOID {$$ = make_default_type(T_VOID);}
              | CHAR {$$ = make_default_type(T_CHAR);}
              | SHORT {$$ = make_default_type(T_SHORT);}
              | INT {$$ = make_default_type(T_INT);}
              | LONG {$$ = make_next_type(T_LONG, NULL);}
              | FLOAT {$$ = make_default_type(T_FLOAT);}
              | DOUBLE{$$ = make_default_type(T_DOUBLE);}
              | SIGNED  {$$ = make_next_type(T_SIGNED, NULL);}
              | UNSIGNED {$$ = make_next_type(T_UNSIGNED, NULL);}
              | _BOOL{
                   yyerror("UNIMPLEMNTED");
                   exit(1);
              }
              | _COMPLEX{
                   yyerror("UNIMPLEMNTED");
                   exit(1);
              }
              | atomic_type_specifier{
                   yyerror("UNIMPLEMNTED");
                   exit(1);
              }
              | struct_or_union_specifier{
                $$ = $1;
              }
              | enum_specifier{
                   yyerror("UNIMPLEMNTED");
                   exit(1);
              }
              | TYPEDEF_NAME{
                   yyerror("UNIMPLEMNTED");
                   exit(1);
              }
              ;

// 6.7.2.1
struct_or_union_specifier: struct_or_union IDENT '{' {create_scope(STRUCT_OR_UNION);} struct_declaration_list '}' {
                            struct SymbolTable * members = shallow_pop_table();
                            struct SymbolTableNode n = make_st_node($2.str, TAGS, TAG, get_default_sc(), make_struct_or_union($1, members), NULL);
                            if(n.name != NULL && !enter_in_namespace(n, TAGS)) {
                                yyerror("Struct Or Union Tag Redefinition");
                                fprintf(stderr, "name = %s\n", n.name);
                                exit(2);
                            };
                            $$ = n.val.type;
                         }// Optional
                         | struct_or_union {create_scope(STRUCT_OR_UNION);}'{' struct_declaration_list '}' {
                            struct SymbolTable * members = shallow_pop_table();
                            struct SymbolTableNode n = make_st_node(NULL, TAGS, TAG, get_default_sc(), make_struct_or_union($1, members), NULL);
                            if(n.name != NULL && !enter_in_namespace(n, TAGS)) {
                                yyerror("Struct Or Union Tag Redefinition");
                                fprintf(stderr, "name = %s\n", n.name);
                                exit(2);
                            };
                            $$ = n.val.type;
                         }
                         | struct_or_union IDENT {
                            struct SymbolTableNode *n;
                            if (!find_in_namespace($2.str, TAGS, &n)) {
                                struct SymbolTableNode node = make_st_node($2.str, TAGS, TAG, get_default_sc(), make_struct_or_union($1, NULL), NULL);
                                enter_in_namespace(node, TAGS); // Should always pass since I checked before
                                find_in_namespace($2.str, TAGS, &n);
                            }
                            $$ = n->val.type;
                        }
                        ;

struct_or_union: STRUCT {$$ = 1;}
               | UNION {$$ = 0;}
               ;

struct_declaration_list: struct_declaration {$$ = NULL;}
                       | struct_declaration_list struct_declaration {$$ = NULL;}
                       ;
struct_declaration: specifier_qualifer_list struct_declarator_list ';' {$$ = NULL;}// Optional
                  | specifier_qualifer_list ';' {fprintf(stderr, "UNIMPLEMNTED"); exit(1);}
                  | static_assert_decleration  {fprintf(stderr, "UNIMPLEMNTED"); exit(1);}
                  ;

specifier_qualifer_list: type_specifier specifier_qualifer_list {
                        $$ = $2;
                        get_last_from_next($$)->extentions.next_type.next = $1;
                       }// Optional
                       | type_specifier {
                        $$ = $1;
                       }
                       | type_qualifier specifier_qualifer_list {
                        $2->qualifier_bit_mask |= $1;
                        $$ = $2;
                       }// Optional
                       | type_qualifier {
                        struct Type * t = make_default_type(0);
                        t->qualifier_bit_mask = $1;
                        $$ = t;
                       } // Optional
                       ;

struct_declarator_list: struct_declarator {
                          struct Type * t = add_to_end_and_reverse($1.node.val.type, $<type>0);
                          $1.node.val.type = t;
                          enter_in_namespace($1.node, MEMS);
                          $$ = $1.node;
                      }
                      | struct_declarator_list ',' struct_declarator {
                          struct Type * t = add_to_end_and_reverse($3.node.val.type, $1.val.type);
                          $3.node.val.type = t;
                          enter_in_namespace($3.node, MEMS);
                          $$ = $3.node;
                      }
                      ;

struct_declarator: declarator {if($1.node.val.type != NULL && $1.node.val.type->type == T_FUNC) {fprintf(stderr, "No Function members\n"); exit(2);} $$ = $1;}
                 | declarator ':' constant_expression {fprintf(stderr, "UNIMPLEMNTED"); exit(1);}// Optional
                 | ':' constant_expression {fprintf(stderr, "UNIMPLEMNTED"); exit(1);}// Optional
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
type_qualifier: CONST {$$ = Q_CONST;}
              | RESTRICT {$$ = Q_RESTRICT;}
              | VOLATILE {$$ = Q_VOLATILE;}
              | _ATOMIC {$$ = Q__ATOMIC;}
              ;

// 6.7.4
function_specifier: INLINE {$$ = F_INLINE;}
                  | _NORETURN {$$ = F__NORETURN;}
                  ;

// 6.7.5
alignment_specifier: _ALIGNAS '(' type_name ')' {
                        yyerror("UNIMPLEMNTED");
                        exit(1);
                   }
                   | _ALIGNAS '(' constant_expression ')'{
                        yyerror("UNIMPLEMNTED");
                        exit(1);
                   }
                   ;

// 6.7.6
declarator: pointer direct_declarator {

          if($2.node.val.type != NULL) {
            struct Type * t = $2.node.val.type->type == T_FUNC ? $2.node.val.type : get_last_from_next($2.node.val.type);
            if (t->type == T_FUNC) {
                t->extentions.func.ret = $1;
                $$ = $2;
            } else {
             get_last_from_next($1)->extentions.next_type.next = $2.node.val.type;
             $2.node.val.type = $1;
             $$ = $2;
            }
          } else {
             get_last_from_next($1)->extentions.next_type.next = $2.node.val.type;
             $2.node.val.type = $1;
             $$ = $2;
          }
          }
          | direct_declarator {
           $$ = $1;
          }
          ;

direct_declarator: IDENT {
                    $$ = make_st_node_pair(make_st_node($1.str, ORD, 0, get_default_sc(), NULL, NULL));
                 }
                 | '(' declarator ')' {
                    $$ = $2; // Double check I interpreted this correctly
                 }
                 | direct_declarator '[' type_qualifier_list assignment_expression ']' {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $1.node.val.type->qualifier_bit_mask = $3;
                    /* $1.val.type = merge_if_next($1.val.type, make_next_type(T_ARR, NULL)); */
                    $1.node.val.type->extentions.next_type.arr_size_expression = $4;
                    $$ = $1;
                 }// Double Optional
                 | direct_declarator '['type_qualifier_list ']' {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $1.node.val.type->qualifier_bit_mask = $3;
                    $$ = $1;
                 }
                 | direct_declarator '[' assignment_expression ']' {
                    /* $1.val.type = merge_if_next($1.val.type, make_next_type(T_ARR, NULL)); */
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $1.node.val.type->extentions.next_type.arr_size_expression = $3;
                    $$ = $1;
                 }
                 | direct_declarator '[' ']' {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $$ = $1;
                 }
                 | direct_declarator '[' STATIC type_qualifier_list assignment_expression']' {
                     fprintf(stderr, "UNIMPLEMNTED");
                     exit(1);
                    /* $1.val.sc = S_STATIC; */
                    /* $1.val.type = make_next_type(T_ARR, $1.val.type); */
                    /* $1.val.type->qualifier_bit_mask = $4; */
                    /* $1.val.type->extentions.next_type.arr_size_expression = $5; */
                    /* $$ = $1; */
                 } // Optional
                 | direct_declarator '[' STATIC assignment_expression']' {
                     fprintf(stderr, "UNIMPLEMNTED");
                     exit(1);
                    /* $1.val.sc = S_STATIC; */
                    /* $1.val.type = make_next_type(T_ARR, $1.val.type); */
                    /* $1.val.type->extentions.next_type.arr_size_expression = $4; */
                    /* $$ = $1; */

                 }
                 | direct_declarator '[' type_qualifier_list STATIC assignment_expression']' {
                     fprintf(stderr, "UNIMPLEMNTED");
                     exit(1);
                    /* $1.val.sc = S_STATIC; */
                    /* $1.val.type = make_next_type(T_ARR, $1.val.type); */
                    /* $1.val.type->qualifier_bit_mask = $3; */
                    /* $1.val.type->extentions.next_type.arr_size_expression = $5; */
                    /* $$ = $1; */
                 }
                 | direct_declarator '[' type_qualifier_list '*' ']'  {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $1.node.val.type->qualifier_bit_mask = $3;
                    $$ = $1;
                 }// Optional
                 | direct_declarator '[' '*' ']' {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $$ = $1;
                 }
                 | direct_declarator '(' {create_scope(PROTOTYPE);} parameter_type_list ')'{
                    struct Type * t = make_func_type(NULL, symbol_table, $4);
                    if ($1.node.val.type != NULL) {
                        struct Type * last = get_last_from_next($1.node.val.type);
                        last->extentions.next_type.next = t;
                        t = $1.node.val.type;
                    } else {
                        $1.st = symbol_table;
                    }
                    $1.node.val.type = t;
                    $1.node.type = FUNCTION;
                    shallow_pop_table();

                    $$ = $1;
                 }
                 | direct_declarator '(' identifier_list ')' {
                    yyerror("K&R Not Supported");
                    exit(2);
                 } // Optional
                 | direct_declarator '(' {create_scope(PROTOTYPE);} ')' {
                    $1.st = shallow_pop_table();
                    $1.node.val.type = make_func_type(NULL, NULL, 0);
                    $1.node.type = FUNCTION;
                    $$ = $1;
                 }
                 ;

pointer: '*' type_qualifier_list { // Optional
        $$ = make_next_type(T_POINTER, NULL);
        $$->qualifier_bit_mask = $2;
       }
       | '*' {
        $$ = make_next_type(T_POINTER, NULL);
       }
       | '*' type_qualifier_list pointer { // Optional
        $$ = make_next_type(T_POINTER, $3);
        $$->qualifier_bit_mask = $2;
       }
       | '*' pointer {
           $$ = make_next_type(T_POINTER, $2);
       }
       ;

type_qualifier_list: type_qualifier
                   | type_qualifier_list type_qualifier {$$ = $1 | $2;}
                   ;

parameter_type_list: parameter_list {$$ = 0;}
                   | parameter_list ',' ELLIPSIS {$$ = 1;}
                   ;

parameter_list: parameter_declaration {
                if(!enter_in_namespace($1.node, ORD)) {
                    yyerror("Redefinition of Identifier");
                };
                $$ = NULL;

              }
              | parameter_list ',' parameter_declaration {
                if(!enter_in_namespace($3.node, ORD)) {
                    yyerror("Redefinition of Identifier");
                };
                $$ = NULL;
              }
              ;

parameter_declaration: declaration_specifiers declarator {
                       struct Type * t = add_to_end_and_reverse($2.node.val.type, $1.val.type);
                       $$ = make_st_node_pair(make_st_node($2.node.name, ORD, VARIABLE, $1.val.sc, t, NULL));
                     }
                     | declaration_specifiers abstract_declarator {
                       struct Type * t = add_to_end_and_reverse($2.node.val.type, $1.val.type);
                       $$ = make_st_node_pair(make_st_node(NULL, ORD, VARIABLE, $1.val.sc, t, NULL));
                     }// Optional
                     | declaration_specifiers {$$ = make_st_node_pair($1);}
                     ;

identifier_list: IDENT
               | identifier_list ',' IDENT
               ;

// 6.7.7
type_name: specifier_qualifer_list abstract_declarator // Optional
         | specifier_qualifer_list
         ;

abstract_declarator: pointer {
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, 0, get_default_sc(), $1, NULL));
                   }
                   | pointer direct_abstract_declarator {
                      if($2.node.val.type != NULL) {
                        struct Type * t = $2.node.val.type->type == T_FUNC ? $2.node.val.type : get_last_from_next($2.node.val.type);
                        if (t->type == T_FUNC) {
                            t->extentions.func.ret = $1;
                            $$ = $2;
                        } else {
                         get_last_from_next($1)->extentions.next_type.next = $2.node.val.type;
                         $2.node.val.type = $1;
                         $$ = $2;
                        }
                      } else {
                         get_last_from_next($1)->extentions.next_type.next = $2.node.val.type;
                         $2.node.val.type = $1;
                         $$ = $2;
                      }
                   }// Optional
                   | direct_abstract_declarator {
                       $$ = $1;
                   }
                   ;

direct_abstract_declarator: '(' abstract_declarator ')' {$$ = $2;}
                          | direct_abstract_declarator '[' type_qualifier_list assignment_expression ']' {

                            $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                            $1.node.val.type->qualifier_bit_mask = $3;
                            /* $1.node.val.type = merge_if_next($1.node.val.type, make_next_type(T_ARR, NULL)); */
                            $1.node.val.type->extentions.next_type.arr_size_expression = $4;
                            $$ = $1;
                          }// Triple Optional
                          | '[' ']' {
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, 0, get_default_sc(), make_next_type(T_ARR, NULL), NULL));
                          }
                          | '[' type_qualifier_list  ']'{
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, 0, get_default_sc(), make_next_type(T_ARR, NULL), NULL));
                             $$.node.val.type->qualifier_bit_mask = $2;
                          }
                          | '[' assignment_expression ']'{
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, 0, get_default_sc(), make_next_type(T_ARR, NULL), NULL));
                             $$.node.val.type->extentions.next_type.arr_size_expression = $2;
                          }
                          | direct_abstract_declarator '[' ']' {
                             $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                             $$ = $1;
                          }
                          | direct_abstract_declarator '[' type_qualifier_list  ']' {
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $1.node.val.type->qualifier_bit_mask = $3;
                    $$ = $1;
                 }
                          | direct_abstract_declarator '['  assignment_expression ']'{
                    /* $1.node.val.type = merge_if_next($1.node.val.type, make_next_type(T_ARR, NULL)); */
                    $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                    $$ = $1;
                 }
                          |  '[' type_qualifier_list assignment_expression ']'{
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, 0, get_default_sc(), make_next_type(T_ARR, NULL), NULL));
                             $$.node.val.type->qualifier_bit_mask = $2;
                             $$.node.val.type->extentions.next_type.arr_size_expression = $3;
                          }
                          | direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }// Double Optional
                          | '[' STATIC  assignment_expression ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }
                          | direct_abstract_declarator '[' STATIC assignment_expression ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }
                          |  '[' STATIC type_qualifier_list assignment_expression ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }
                          | direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']'  {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }// Optional
                          |  '[' type_qualifier_list STATIC assignment_expression ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }
                          | direct_abstract_declarator '[' '*' ']' {

                            $1.node.val.type = make_next_type(T_ARR, $1.node.val.type);
                            $$ = $1;
                          }// Optional
                          |  '[' '*' ']' {
                             fprintf(stderr, "UNIMPLEMNTED");
                             exit(1);
                          }
                          | direct_abstract_declarator '(' {create_scope(PROTOTYPE);}parameter_type_list ')'  {
                            struct Type * t = make_func_type(NULL, symbol_table, $4);
                            if ($1.node.val.type != NULL) {
                                struct Type * last = get_last_from_next($1.node.val.type);
                                last->extentions.next_type.next = t;
                                t = $1.node.val.type;
                            } else {
                                $1.st = symbol_table;
                            }
                            $1.node.val.type = t;
                            $1.node.type = FUNCTION;
                            shallow_pop_table();
                            $$ = $1;
                          }// Double Optional
                          | direct_abstract_declarator '(' {create_scope(PROTOTYPE);} ')' {
                            $1.st = shallow_pop_table();
                            $1.node.val.type = make_func_type(NULL, NULL, 0);
                            $1.node.type = FUNCTION;
                            $$ = $1;
                          }
                          |  '(' {create_scope(PROTOTYPE);} parameter_type_list ')'{
                              $$ = make_st_node_pair(make_st_node(NULL, ORD, FUNCTION, get_default_sc(), make_func_type(NULL, symbol_table, $3), NULL));
                              $$.st = shallow_pop_table();
                          }
                          | '('{create_scope(PROTOTYPE);} ')' {
                             $$ = make_st_node_pair(make_st_node(NULL, ORD, FUNCTION, get_default_sc(), make_func_type(NULL, NULL, 0), NULL));
                             $$.st = shallow_pop_table();
                          }
                          ;

// 6.7.8
/* typedef_name: IDENT ; */

// 6.7.9
initalizer:  assignment_expression
          | '{' initalizer_list '}' {fprintf(stderr, "UNIMPLEMNTED");exit(1);}
          | '{' initalizer_list ',' '}' {fprintf(stderr, "UNIMPLEMNTED");exit(1);}
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
        | declaration
        | compound_statment
        | expression_statment
        | selection_statment
        | iteration_statment
        | jump_statment
        ;

// 6.8.1
labeled_statment: IDENT ':' statment {
                    struct SymbolTableNode *n = NULL;
                    if(!enter_in_namespace(make_st_node($1.str, LABEL,
                                    LABELNAME,
                                    get_default_sc(), make_label_type(1),
                                    NULL), LABEL)) {
                        find_in_namespace($1.str, LABEL, &n);
                        if(n->val.type->extentions.label.initalized) {
                            yyerror("Redefinition of Label");
                            exit(2);
                        } else {
                            n->val.type->extentions.label.initalized = 1;
                            AstNode * label_node = make_LabelStatment(n);
                            AstNode * list = make_StatementList(label_node);
                            append_StatmentList(&list->statments, $3);
                            $$ = list;
                        }
                    } else {
                        if(!find_in_namespace($1.str, LABEL, &n)) {
                            fprintf(stderr, "This should never happend");
                            exit(1);
                        };
                        AstNode * label_node = make_LabelStatment(n);
                        AstNode * list = make_StatementList(label_node);
                        append_StatmentList(&list->statments, $3);
                        $$ = list;
                    }
                }
                | CASE constant_expression ':' statment {fprintf(stderr, "debug\n");$$ = make_CaseStatment($2, $4);fprintf(stderr, "hi\n");}
                | DEFAULT ':' statment {$$ = make_DefaultStatment($3);}
                ;

// 6.8.2
compound_statment: '{' {create_scope(BLOCK);} block_item_list '}' { // Optional
                    $$ = $3;
                    shallow_pop_table();
                 }
                 | '{' '}' {$$ = NULL;}
                 ;

block_item_list: block_item {$$ = make_StatementList($1);}
               | block_item_list block_item {append_StatmentList(&$1->statments, $2); $$ = $1;}
               ;

block_item: statment
          ;

// 6.8.3
expression_statment: expression ';'
                   | ';' {$$ = NULL;}
                   ;

// 6.8.4
selection_statment: IF '(' expression ')' statment {$$ = make_IfStatment($3, $5, NULL);}
                  | IF '(' expression ')' statment ELSE statment {$$ = make_IfStatment($3, $5, $7);}
                  | SWITCH '(' expression ')' statment  {$$ = make_SwitchStatment($3, $5);}
                  ;

make_block: {create_scope(BLOCK);};

iteration_statment: WHILE '(' expression ')' statment {$$ = make_WhileStatment($3, $5, 0);}
                  | DO statment WHILE '(' expression ')' ';' {$$ = make_WhileStatment($5, $2, 1);}
                  | FOR '(' expression ';' expression ';' expression ')' statment  {$$ = make_ForStatment($3, $5, $7, $9);}// Triple Optional
                  | FOR '(' ';' ';' ')' statment {$$ = make_ForStatment(NULL, NULL, NULL, $6);}
                  | FOR '(' expression ';'  ';'  ')' statment {$$ = make_ForStatment($3, NULL, NULL, $7);}
                  | FOR '('  ';' expression ';'  ')' statment {$$ = make_ForStatment(NULL, $4, NULL, $7);}
                  | FOR '('  ';'  ';' expression ')' statment {$$ = make_ForStatment(NULL, NULL, $5, $7);}
                  | FOR '(' expression ';'  ';' expression ')' statment {$$ = make_ForStatment($3, NULL, $6, $8);}
                  | FOR '(' expression ';' expression ';'  ')' statment {$$ = make_ForStatment($3, $5, NULL, $8);}
                  | FOR '('  ';' expression ';' expression ')' statment {$$ = make_ForStatment(NULL, $4, $6, $8);}
                  | FOR '(' make_block declaration expression ';' expression ')' statment {$$ = make_ForStatment($4, $5, $7, $9); shallow_pop_table();}// Double Optional
                  | FOR '(' make_block declaration  ';'  ')' statment {$$ = make_ForStatment($4, NULL, NULL, $7); shallow_pop_table();}
                  | FOR '(' make_block declaration expression ';'  ')' statment {$$ = make_ForStatment($4, $5, NULL, $8); shallow_pop_table();}
                  | FOR '(' make_block declaration  ';' expression ')' statment {$$ = make_ForStatment($4, NULL, $6, $8); shallow_pop_table();}
                  ;

// 6.8.6
jump_statment: GOTO IDENT ';' {
                struct SymbolTableNode *n;
                if(!find_in_namespace($2.str, LABEL,&n)) {
                    enter_in_namespace(make_st_node($2.str, LABEL,
                                    LABELNAME,
                                    get_default_sc(), make_label_type(0),
                                    NULL), LABEL);
                    find_in_namespace($2.str, LABEL, &n);
                };
                $$ = make_GotoStatment(n);
             }
             | CONTINUE ';' {$$ = make_AstNode(ASTNODE_CONTINUE_STATMENT);}
             | BREAK ';'{$$ = make_AstNode(ASTNODE_BREAK_STATMENT);}
             | RETURN expression ';' {$$ = make_ReturnStatment($2);}// Optional
             | RETURN ';' {$$ = make_ReturnStatment(NULL);}
             ;

// 6.9
translation_unit: external_declaration
                | translation_unit external_declaration
                ;

external_declaration: function_definition
                    | declaration
                    ;
// 6.9.1


// I made this myself
function_compount_statment: '{' {symbol_table = $<st_node_pair>0.st; symbol_table->st_type = FUNC;} block_item_list '}' {shallow_pop_table(); $$ = $3;}
                          | '{' '}' {$$ = NULL;}
                          ;

function_definition: declaration_specifiers declarator declaration_list compound_statment {
                    yyerror("K&R Not Supported");
                    exit(2);
                 }// Optional
                   | declaration_specifiers declarator function_compount_statment{
                    struct Type *t = add_to_end_and_reverse($2.node.val.type, $1.val.type);
                    if (t->type != T_FUNC || $2.node.namespc != ORD) {
                        yyerror("Invalid Funciton Definiton");
                        exit(2);
                    }
                    struct SymbolTableNode current_node = make_st_node($2.node.name, $2.node.namespc, $2.node.type, $1.val.sc,t, NULL);
                    // find in symbol table and attach compound_statment
                    // or enter in namespace
                    struct SymbolTableNode * old_node;
                    int found;
                    if((found = find_in_table($2.node.name, ORD,symbol_table, &old_node)) && func_is_comp(old_node->val.type, current_node.val.type)) {
                        old_node->val.type->extentions.func.statment = $3;
                    } else if (!found) {
                        current_node.val.type->extentions.func.statment = $3;
                        enter_in_namespace(current_node, ORD);
                    } else {
                        yyerror("funciton signature is not compatible");
                        exit(2);
                    }

                   }
                   ;

declaration_list: declaration
                | declaration_list declaration
                ;

