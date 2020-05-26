#ifndef __SEMANTIC_H__
#define __SEMANTIC_H__

#include "Node.h"
#include "rb_tree.h"
#include <string.h>
#include "common.h"

#define MAX_NAME_SIZE 256
extern int semerr;

typedef struct Type Type;
typedef struct FieldList FieldList;
typedef struct ArgList ArgList;
typedef struct Func Func;
typedef struct Symbol Symbol;

typedef enum SymbolKind { SYM_VAR, SYM_FUNC, SYM_STRUCT } SymbolKind;
typedef enum TypeKind { BASIC, ARRAY, STRUCTURE } TypeKind;
typedef enum BasicType { TYPE_INT, TYPE_FLOAT } BasicType;

/* RB_Tree *symbolTable = NULL; */
/* RB_Tree *symbolTable = createRB_Tree(symbolCmp); */

struct Type {
    TypeKind kind;  // basic / array / structure
    union {
        BasicType basic;
        struct {
            Type *elem;
            int size;
        } array;
        FieldList *structure;
    } u;
};

struct FieldList {  // structure field
    Type *type;
    char name[MAX_NAME_SIZE];
    FieldList *next;
};

struct ArgList {    // function arguments type
    Type *type;
    ArgList *next;
};

struct Func {
    Type *retType;
    ArgList *argList;
};

struct Symbol {
    SymbolKind kind;    // variable / function / structure
    char name[MAX_NAME_SIZE];
    union {
        Type *type; // variable and structure
        Func *func; // function
    } u;
};

void semantic_parse(Node *root);
Symbol* lookupSymbol(const char*name, SymbolKind kind);
Type* getExpType(Node *exp);    // for struct and array use
void clearSymbolTable();
#endif
