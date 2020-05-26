#include "Node.h"
#include <string.h>
#include <assert.h>

// for print parse tree (lab1)
static const char* TypeName[] =  {
    "INT",        "FLOAT",      "SEMI",
    "COMMA",      "ASSIGNOP",   "RELOP",
    "PLUS",       "MINUS",      "STAR",
    "DIV",        "AND",        "OR",
    "DOT",        "NOT",        "LP",
    "RP",         "LB",         "RB",
    "LC",         "RC",         "TYPE",
    "STRUCT",     "RETURN",     "IF",
    "ELSE",       "WHILE",      "ID",
    "Error",
    "Program",    "ExtDefList", "ExtDef",
    "ExtDecList", "Specifier",  "StructSpecifier",
    "OptTag",     "Tag",        "VarDec",
    "FunDec",     "VarList",    "ParamDec",
    "CompSt",     "StmtList",   "Stmt",
    "DefList",    "Def",        "DecList",
    "Dec",        "Exp",        "Args"
};

Node* createNode(NodeType type, int lineno) {
/* Node* createNode(const char* type, int lineno) { */
    Node* newNode = (Node*)malloc(sizeof(Node));
    memset(newNode, 0, sizeof(Node));
    newNode->type = type;
    /* strncpy(newNode->type, type, strlen(type) + 1); */ 
    memset(&newNode->val, 0, sizeof(newNode->val)); // pay attention to the operator precendece
    /* newNode->field = NULL; */
    /* strncpy(newNode->text, text, strlen(text)); */
    newNode->lineno = lineno;
    newNode->child = newNode->sib = NULL;
    newNode->childno = 0;
    return newNode;
}

void addChild(Node* pr, int cnt, ...) {
    if(pr == NULL || cnt < 1) return;
    /* printf("cnt = %d\n", cnt); */
    va_list ap;
    va_start(ap, cnt); 
    pr->childno += cnt;
    for(int i = 0; i < cnt; i++) {
        Node *p = va_arg(ap, Node*);
        // empty production will not be insert
        if(p != NULL) {
            p->sib = pr->child;
            pr->child = p;
        }
    }
    va_end(ap);
}

void traverseTree(Node *node, int depth) {
   if(node == NULL) return; 

   if(node->type > NODE_Program && node->child == NULL) return;
   /* print blanks */
   for(int i = 0; i < depth * 2; i++) {
       printf(" ");
   }
   if(node->child == NULL) {    /* terminal symbol */
       switch(node->type) {
            case NODE_ID: case NODE_TYPE:
                printf("%s: %s\n", TypeName[node->type], node->val.name);
                break;
            case NODE_INT:
                printf("%s: %d\n", TypeName[node->type], node->val.intVal);
                break;
            case NODE_FLOAT:
                printf("%s: %f\n", TypeName[node->type], node->val.floatVal);
                break;
            default:
                printf("%s\n", TypeName[node->type]);
       }
       /* if(strcmp(node->type, "ID") == 0 */
       /*         || strcmp(node->type, "TYPE") == 0) { */
       /*     printf("%s: %s\n", node->type, node->val.name); */
       /* } else if(strcmp(node->type, "INT") == 0) { */
       /*     printf("%s: %d\n", node->type, node->val.intVal); */
       /* } else if(strcmp(node->type, "FLOAT") == 0) { */
       /*     printf("%s: %f\n", node->type, node->val.floatVal); */
       /* } else { */
       /*     printf("%s\n", node->type); */
       /* } */
   } else { /* nonterminal symbol */
        printf("%s (%d)\n", TypeName[node->type], node->lineno);
   }

   Node* p = node->child;
   while(p != NULL) {
       traverseTree(p, depth+1);
       p = p->sib;
   }
}

void freeTree(Node *node) {
    if(node == NULL) return;
    Node* p = node->child;
    while(p != NULL) {
        freeTree(p);
        p = p->sib;
    }
    /* if(node->field != NULL) { */
    /*     if(node->field->string != NULL) { */
    /*         free(node->field->string); */
    /*     } */
    /*     free(node->field); */
    /* } */
    free(node);
}

Node* getNthChild(Node *node, int n) {
    assert(node);
    Node *iter = node->child;
    for(int i = 0; i < n - 1; i++) {
        assert(iter);
        iter = iter->sib;
    }
    return iter;
}

