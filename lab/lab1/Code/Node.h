#ifndef __NODE_H__
#define __NODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define SIZE 32

extern int yylineno;
/* typedef enum Type { */
/*     DEFAULT, INT, FLOAT, STRING */
/* } Type; */
typedef struct Field {
    int intVal;
    float floatVal;
    char* string;
} Field;
typedef struct Node {
    int lineno;
    struct Node* firstChild;
    struct Node* nextSibling;
    char type[SIZE];
    Field *field;
    /* char text[SIZE]; */
    /* Type type; */
    /* union { */
    /*     int intVal; */
    /*     float floatVal; */
    /*     char* strVal; */
    /* }; */
} Node;

void addChild(Node *pr, int cnt, ...);
Node* createNode(const char* type, int lineno);
/* void traverseTree(Node *node, int level); */
void traverseTree(Node *node, int blanks);
void freeTree(Node* node);

#endif
