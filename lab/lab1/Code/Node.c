#include "Node.h"
#include <string.h>

/* Node* createNode(const char* type, const char* text, int lineno) { */
/*     Node* newNode = (Node*)malloc(sizeof(Node)); */
/*     strncpy(newNode->type, type, strlen(type)); */ 
/*     strncpy(newNode->text, text, strlen(text)); */
/*     newNode->lineno = lineno; */
/*     newNode->firstChild = newNode->nextSibling = NULL; */
/*     return newNode; */
/* } */
Node* createNode(const char* type, int lineno) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    memset(newNode, 0, sizeof(Node));
    strncpy(newNode->type, type, strlen(type) + 1); 
    newNode->field = NULL;
    /* strncpy(newNode->text, text, strlen(text)); */
    newNode->lineno = lineno;
    newNode->firstChild = newNode->nextSibling = NULL;
    return newNode;
}

void addChild(Node* pr, int cnt, ...) {
    if(pr == NULL || cnt < 1) return;
    /* printf("cnt = %d\n", cnt); */
    va_list ap;
    va_start(ap, cnt); 
    for(int i = 0; i < cnt; i++) {
        Node *p = va_arg(ap, Node*);
        if(p != NULL) {
            p->nextSibling = pr->firstChild;
            pr->firstChild = p;
         }
    }
    va_end(ap);
}

void traverseTree(Node *node, int depth) {
   if(node == NULL) return; 

   /* print blanks */
   for(int i = 0; i < depth * 2; i++) {
       printf(" ");
   }
   if(node->firstChild == NULL) {    /* terminal symbol */
       if(strcmp(node->type, "ID") == 0
               || strcmp(node->type, "TYPE") == 0) {
           printf("%s: %s\n", node->type, node->field->string);
       } else if(strcmp(node->type, "INT") == 0) {
           printf("%s: %d\n", node->type, node->field->intVal);
       } else if(strcmp(node->type, "FLOAT") == 0) {
           printf("%s: %f\n", node->type, node->field->floatVal);
       } else {
           printf("%s\n", node->type);
       }
   } else { /* nonterminal symbol */
        printf("%s (%d)\n", node->type, node->lineno);
        /* Node* p = node->firstChild; */
        /* while(p != NULL) { */
        /*     traverseTree(p); */
        /*     p = p-> */
        /* } */
   }

   Node* p = node->firstChild;
   while(p != NULL) {
       traverseTree(p, depth+1);
       p = p->nextSibling;
   }
}

void freeTree(Node *node) {
    if(node == NULL) return;
    Node* p = node->firstChild;
    while(p != NULL) {
        freeTree(p);
        p = p->nextSibling;
    }
    if(node->field != NULL) {
        if(node->field->string != NULL) {
            free(node->field->string);
        }
        free(node->field);
    }
    free(node);
}

