#pragma once
#include "./yylval_types.h"
#include "symbol_table.h"

#define ASTNODE_CONSTANT 1
#define ASTNODE_STRLIT 2
#define ASTNODE_IDENT 3 // TODO make this with scopes and stuff

#define ASTNODE_UNARYOP 4
struct UnaryOp {
    int op;
    struct AstNode *child;
};

#define ASTNODE_BINARYOP 5
struct BinaryOp {
    int op;
    struct AstNode *left;
    struct AstNode *right;
};
#define ASTNODE_TERNAYROP 6
struct TernayOp {
    struct AstNode *cond;
    struct AstNode *truthy;
    struct AstNode *falsey;
};

#define ASTNODE_FUNCCALL 7

struct AstNodeListNode {
    struct AstNode *val;
    struct AstNodeListNode *next;
};
struct FuncCall {
    struct AstNode *name;
    int argument_count;
    struct AstNodeListNode arguments;
};

#define ASTNODE_DECLARATORLIST 8
struct DeclaratorList {
    struct DeclaratorListNode *tail;
    struct DeclaratorListNode *head;
};

struct DeclaratorListNode {
    struct SymbolTableNode *node;
    struct DeclaratorListNode *next;
};

struct AstNode {
    int type;
    union {
        YYlvalNumLit constant;
        YYlvalStrLit strlit;
        char *ident;
        struct UnaryOp unary_op;
        struct BinaryOp binary_op;
        struct TernayOp ternary_op;
        struct FuncCall func_call;
        struct DeclaratorList declarator_list;
    };
};

typedef struct AstNode AstNode;

void print_AstNode(AstNode *head, unsigned int tab_count);

AstNode *make_func_call(AstNode *name, struct AstNodeListNode *arguments);

struct AstNodeListNode *make_node_list_node(AstNode *node);

struct AstNodeListNode *append_AstNodeListNode(struct AstNodeListNode *,
                                               AstNode *next);

AstNode *make_DeclaratorList(struct SymbolTableNode *n);

void append_DeclaratorList(AstNode *declarator_list, struct SymbolTableNode *n);

AstNode *make_IdentNode(YYlvalStrLit val);

AstNode *make_AstNode(int type);

AstNode *make_binary_op(int op, AstNode *left, AstNode *right);

AstNode *make_unary_op(int op, AstNode *child);

AstNode *make_ternary_op(AstNode *cond, AstNode *truthy, AstNode *falsey);
