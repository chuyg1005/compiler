#ifndef __NODE_H__
#define __NODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define SIZE 32
#define MAX_TOKEN_SIZE 256

extern int yylineno;

typedef enum NodeType {
    NODE_INT = 0,    NODE_FLOAT,      NODE_SEMI,
    NODE_COMMA,      NODE_ASSIGNOP,   NODE_RELOP,
    NODE_PLUS,       NODE_MINUS,      NODE_STAR,
    NODE_DIV,        NODE_AND,        NODE_OR,
    NODE_DOT,        NODE_NOT,        NODE_LP,
    NODE_RP,         NODE_LB,         NODE_RB,
    NODE_LC,         NODE_RC,         NODE_TYPE,
    NODE_STRUCT,     NODE_RETURN,     NODE_IF,
    NODE_ELSE,       NODE_WHILE,      NODE_ID,
    NODE_Error,
    NODE_Program,    NODE_ExtDefList, NODE_ExtDef,
    NODE_ExtDecList, NODE_Specifier,  NODE_StructSpecifier,
    NODE_OptTag,     NODE_Tag,        NODE_VarDec,
    NODE_FunDec,     NODE_VarList,    NODE_ParamDec,
    NODE_CompSt,     NODE_StmtList,   NODE_Stmt,
    NODE_DefList,    NODE_Def,        NODE_DecList,
    NODE_Dec,        NODE_Exp,        NODE_Args
} NodeType;

// REFACTOR NODE STRUCTURE
typedef struct Node {
    int childno;
    int lineno;
    struct Node* child;
    struct Node* sib;
    NodeType type;
    union {
        int intVal;
        float floatVal;
        char name[MAX_TOKEN_SIZE];
    } val;
} Node;

void addChild(Node *pr, int cnt, ...);
/* Node* createNode(const char* type, int lineno); */
Node* createNode(NodeType type, int lineno);
/* Node* getNthChild(Node *node, int n); */
/* void traverseTree(Node *node, int level); */
void traverseTree(Node *node, int blanks);
void freeTree(Node* node);

#endif
