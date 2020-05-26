#include "rb_tree.h"
#include <stdio.h>

// RB_Tree private functions
static void clear(RB_Node *, FreeEntry freeEntry);
static bool isRed(RB_Node *);
static bool isBlack(RB_Node *);
static RB_Node *find(RB_Tree *self, Entry entry);
static RB_Node *brother(RB_Node *);
static RB_Node *rotateLeft(RB_Tree *self, RB_Node *node);
static RB_Node *rotateRight(RB_Tree *self, RB_Node *node);
static void fixAfterInsertion(RB_Tree *self, RB_Node *node);
static void fixAfterDeletion(RB_Tree *self, RB_Node *node);

// RB_Node private functions
static RB_Node *createRB_Node(Entry entry, Color color);
static RB_Node *succ(RB_Node *self);
static RB_Node *prev(RB_Node *self);
static RB_Node *next(RB_Node *self);

// RB_Iterator private functions
static RB_Iterator* createRB_Iterator(RB_Node *node);
static int checkRB_Tree(RB_Tree *self, RB_Node *node);

bool hasNext(RB_Iterator *rb_iterator)
{
    return rb_iterator->node != NULL;
}

Entry nextNode(RB_Iterator *rb_iterator)
{
    RB_Node *prev = rb_iterator->node;
    if (prev != NULL) {
        rb_iterator->node = next(prev);
    }
    return prev->entry;
}
static RB_Iterator* createRB_Iterator(RB_Node *node) {
    RB_Iterator *iter = (RB_Iterator*)malloc(sizeof(RB_Iterator));
    iter->node = node;
    return iter;
}

static RB_Node *createRB_Node(Entry entry, Color color)
{
    RB_Node *rb_node = (RB_Node *)malloc(sizeof(RB_Node));
    rb_node->entry = entry;
    rb_node->color = color;
    rb_node->father = NULL;
    rb_node->leftChild = NULL;
    rb_node->rightChild = NULL;
    return rb_node;
}
static RB_Node *succ(RB_Node *self)
{
    /* assert(node); */
    assert(self->rightChild);
    /* assert(this->rightChild); */
    RB_Node *p = self->rightChild;
    while (p->leftChild)
    {
        p = p->leftChild;
    }
    return p;
}
static RB_Node *prev(RB_Node *self)
{
    RB_Node *p = self;
    if (!p->leftChild)
    { // no left child
        while (p->father && p == p->father->leftChild)
        {
            p = p->father;
        }
        p = p->father;
    }
    else
    {
        p = p->leftChild;
        while (p->rightChild)
        {
            p = p->rightChild;
        }
    }
    return p;
}
static RB_Node *next(RB_Node *self)
{
    RB_Node *p = self;
    if (!p->rightChild)
    { // no right child
        while (p->father && p == p->father->rightChild)
        {
            p = p->father;
        }
        p = p->father;
    }
    else
    {
        p = succ(self);
    }
    return p;
}

RB_Tree *createRB_Tree(Comparator comparator)
{
    RB_Tree *rb_tree = (RB_Tree *)malloc(sizeof(RB_Tree));
    rb_tree->root = NULL;
    rb_tree->hot = NULL;
    rb_tree->comparator = comparator;
    rb_tree->size = 0;
    return rb_tree;
}

void clearRB_Tree(RB_Tree *self, FreeEntry freeEntry)
{
    clear(self->root, freeEntry);
    self->root = NULL;
    self->hot = NULL;
    self->comparator = NULL;
    self->size = 0;
}

static void clear(RB_Node *node, FreeEntry freeEntry)
{
    if (node)
    {
        clear(node->leftChild, freeEntry);
        clear(node->rightChild, freeEntry);
        freeEntry(node->entry);
        free(node);
        /* delete node; */
    }
}

RB_Iterator* iterator(RB_Tree *self)
{
    if (self->size == 0) {
        return createRB_Iterator(NULL);
    }
    /* return {.node = NULL}; */
    RB_Node *node = self->root;
    while (node->leftChild)
    {
        node = node->leftChild;
    }
    return createRB_Iterator(node);
}

static bool isRed(RB_Node *node)
{
    // null node is black
    return node && node->color == RED;
}

static bool isBlack(RB_Node *node)
{
    return !node || node->color == BLACK;
}

static RB_Node *brother(RB_Node *node)
{
    assert(node && node->father);
    if (node == node->father->leftChild)
    {
        return node->father->rightChild;
    }
    return node->father->leftChild;
}

static RB_Node *find(RB_Tree *self, Entry entry)
{
    RB_Node *node = self->root;
    self->hot = NULL;
    int r;
    while (node && (r = self->comparator(entry, node->entry)) != 0)
    {
        self->hot = node;
        if (r < 0)
        {
            node = node->leftChild;
        }
        else
        {
            node = node->rightChild;
        }
    }
    return node;
}

Entry RB_find(RB_Tree *self, Entry entry) {
    RB_Node* node = find(self, entry);
    return node == NULL ? NULL : node->entry;
}

static RB_Node *rotateLeft(RB_Tree *self, RB_Node *node)
{
    // right child to father
    node->rightChild->father = node->father;
    if (node->father)
    {
        if (node->father->leftChild == node)
        {
            node->father->leftChild = node->rightChild;
        }
        else
        {
            node->father->rightChild = node->rightChild;
        }
    }
    else
    {
        self->root = node->rightChild;
    }
    // right child's left child
    if (node->rightChild->leftChild)
    {
        node->rightChild->leftChild->father = node;
    }
    node->father = node->rightChild;
    node->rightChild = node->rightChild->leftChild;
    node->father->leftChild = node;
    /* RB_check(self); */
    return node->father; // return new root
}

static RB_Node *rotateRight(RB_Tree *self, RB_Node *node)
{
    node->leftChild->father = node->father;
    if (node->father)
    {
        if (node->father->leftChild == node)
        {
            node->father->leftChild = node->leftChild;
        }
        else
        {
            node->father->rightChild = node->leftChild;
        }
    }
    else
    {
        self->root = node->leftChild;
    }
    if (node->leftChild->rightChild)
    {
        node->leftChild->rightChild->father = node;
    }
    node->father = node->leftChild;
    node->leftChild = node->leftChild->rightChild;
    node->father->rightChild = node;
    /* RB_check(self); */
    return node->father;
}

bool RB_insert(RB_Tree *self, Entry entry)
{
    RB_Node *node = find(self, entry);
    // already exist
    if (node)
    {
        return false;
    }
    if (!self->hot)
    {
        assert(self->size == 0);
        self->root = createRB_Node(entry, BLACK);
        self->size = 1;
        return true;
    }
    ++self->size;
    node = createRB_Node(entry, RED);
    node->father = self->hot;
    if (self->comparator(entry, self->hot->entry) < 0)
    {
        self->hot->leftChild = node;
    }
    else
    {
        self->hot->rightChild = node;
    }
    /* solveDoubleRed(node); */
    fixAfterInsertion(self, node);
    return true;
}

static void fixAfterInsertion(RB_Tree *self, RB_Node *node)
{
    while (node->father && node->father->color == RED)
    {
        RB_Node *father = node->father;
        RB_Node *grandFather = father->father;
        RB_Node *uncle = brother(father);

        // case 1: uncle is red
        if (isRed(uncle))
        {
            uncle->color = BLACK;
            father->color = BLACK;
            grandFather->color = RED;
            node = grandFather;
        }
        else
        {
            if (father == grandFather->leftChild)
            {
                if (node == father->rightChild)
                {
                    rotateLeft(self, father);
                    node = father;
                    father = node->father;
                }
                grandFather->color = RED;
                father->color = BLACK;
                rotateRight(self, grandFather);
            }
            else
            {
                if (node == father->leftChild)
                {
                    rotateRight(self, father);
                    node = father;
                    father = node->father;
                }
                grandFather->color = RED;
                father->color = BLACK;
                rotateLeft(self, grandFather);
            }
        }
    }
    self->root->color = BLACK;
}

bool RB_remove(RB_Tree *self, Entry entry)
{
    RB_Node *node = find(self, entry);
    RB_Node *succ_node = NULL;
    if (!node)
    {
        return false;
    }
    --self->size;
    // find successor to substitute the deleted node
    if (node->leftChild && node->rightChild)
    {
        succ_node = succ(node);
        node->entry = succ_node->entry;
        node = succ_node;
    }
    if (node->leftChild)
    {
        node->entry = node->leftChild->entry;
        node = node->leftChild;
    }
    else if (node->rightChild)
    {
        node->entry = node->rightChild->entry;
        node = node->rightChild;
    }
    // only black need to fix
    if (node->color == BLACK)
    {
        /* solveLostBlack(node); */
        fixAfterDeletion(self, node);
    }
    // pick up node from tree
    if (node->father)
    {
        if (node == node->father->leftChild)
        {
            node->father->leftChild = NULL;
        }
        else
        {
            node->father->rightChild = NULL;
        }
    }
    else
    {
        self->root = NULL;
    }
    free(node);
    /* delete node; */
    return true;
}

static void fixAfterDeletion(RB_Tree *self, RB_Node *node)
{
    // not root
    while (node->father && node->color == BLACK)
    {
        RB_Node *father = node->father;
        RB_Node *sibling = brother(node);

        if (node == father->leftChild)
        {
            // case 1: siblingther is red
            if (isRed(sibling))
            {
                sibling->color = BLACK;
                father->color = RED;
                rotateLeft(self, father);
                sibling = brother(node);
            }

            // siblingther is black
            if (isBlack(sibling->leftChild) && isBlack(sibling->rightChild))
            {
                sibling->color = RED;
                node = father;
            }
            else
            {
                if (isBlack(sibling->rightChild))
                {
                    sibling->leftChild->color = BLACK;
                    sibling->color = RED;
                    rotateRight(self, sibling);
                    sibling = brother(node);
                }
                sibling->color = father->color;
                father->color = BLACK;
                sibling->rightChild->color = BLACK;
                rotateLeft(self, father);
                node = self->root;
            }
        }
        else
        {
            if (isRed(sibling))
            {
                sibling->color = BLACK;
                father->color = RED;
                rotateRight(self, father);
                sibling = brother(node);
            }

            if (isBlack(sibling->leftChild) && isBlack(sibling->rightChild))
            {
                sibling->color = RED;
                node = father;
            }
            else
            {
                if (isBlack(sibling->leftChild))
                {
                    sibling->rightChild->color = BLACK;
                    sibling->color = RED;
                    rotateLeft(self, sibling);
                    sibling = brother(node);
                }
                sibling->color = father->color;
                father->color = BLACK;
                sibling->leftChild->color = BLACK;
                rotateRight(self, father);
                node = self->root;
            }
        }
    }
    node->color = BLACK;
}

static int checkRB_Tree(RB_Tree *self, RB_Node *node) {
    /* assert(0); */
    if(!node) return 1; // nil node

    // check red and black
    if(isRed(node)) {
        if(isRed(node->leftChild) || isRed(node->rightChild)) {
            assert(0);
            /* return false; */
        }
    }

    if((node->leftChild && node->leftChild->father != node)
            || (node->rightChild && node->rightChild->father != node)) {
        assert(0);
    }

    if((node->leftChild&& self->comparator(node->leftChild->entry, node->entry) > 0)
            || (node->rightChild && self->comparator(node->rightChild->entry, node->entry) < 0)) {
        assert(0);
        /* return false; */
    }
    int leftBlackHeight = checkRB_Tree(self, node->leftChild);
    int rightBlackHeight = checkRB_Tree(self, node->leftChild);
    if(leftBlackHeight != rightBlackHeight) assert(0);

    if(isRed(node)) {
        return leftBlackHeight;
    } else {
        return leftBlackHeight + 1;
    }
    /* return checkRB_Tree(self, node->leftChild) && checkRB_Tree(self, node->rightChild); */
}

void RB_check(RB_Tree *self) {
    /* int h = checkRB_Tree(self, self->root); */
    checkRB_Tree(self, self->root);
    /* printf("rb tree black height = %d.\n", h); */
}
