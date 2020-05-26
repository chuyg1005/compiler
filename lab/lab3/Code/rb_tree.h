#ifndef __RB_TREE__
#define __RB_TREE__
#include <assert.h>
#include <stdlib.h>
#include "common.h"

typedef struct Symbol Symbol;
typedef Symbol* Entry;  
/* typedef int Entry; */
typedef struct RB_Node RB_Node;
typedef struct RB_Tree RB_Tree;
typedef struct RB_Iterator RB_Iterator;
typedef int (*Comparator)(Entry entry1, Entry entry2);  // for compare entry
typedef void (*FreeEntry)(Entry entry); // for deallocate rb-tree

typedef enum Color {
    RED, BLACK
} Color;

struct RB_Node {
    Entry entry;
    /* Key key; */
    /* Value value; */
    Color color;
    RB_Node *father;
    RB_Node *leftChild;
    RB_Node *rightChild;
};

struct RB_Iterator {
    RB_Node *node;
};

struct RB_Tree {
    RB_Node *root;
    RB_Node *hot;
    int size;
    Comparator comparator;
};

RB_Tree* createRB_Tree(Comparator comparator);
void clearRB_Tree(RB_Tree *self, FreeEntry freeEntry);
bool RB_insert(RB_Tree *self, Entry entry);    // key and value
bool RB_remove(RB_Tree *self, Entry entry);
Entry RB_find(RB_Tree *self, Entry entry);
RB_Iterator* iterator(RB_Tree *self);
bool hasNext(RB_Iterator*);
Entry nextNode(RB_Iterator*);
void RB_check(RB_Tree *self);
/* int checkRB_Tree(RB_Tree *self, RB_Node *node); // for debug */
// RB_Iterator end(RB_Tree *self);
#endif
