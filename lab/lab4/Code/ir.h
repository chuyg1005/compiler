#ifndef __IR_H__
#define __IR_H__
#include "common.h"
#include "semantic.h"
#include "stdio.h"

typedef struct Operand Operand;
typedef struct IR IR;   // intermediate code
typedef struct ArgNode ArgNode;
typedef struct IRList IRList;   // intermediate code list
typedef struct DeadCode DeadCode;   // for optimization

typedef enum { 
    OP_TEMP, 
    OP_VAR, 
    OP_CONST, 
    OP_LABEL, 
    OP_FUNC,
    OP_INV  // invalid operand, for code optimization
} OperandKind;
typedef enum { 
    IR_LABEL,
    IR_FUNC, 
    IR_ASSIGN, 
    IR_ADD, 
    IR_SUB, 
    IR_MUL, 
    IR_DIV, 
    IR_REF, 
    IR_DEREF_L, 
    IR_DEREF_R, 
    IR_GOTO, 
    IR_RELOP,
    IR_RET, 
    IR_DEC, 
    IR_ARG, 
    IR_CALL, 
    IR_PARM, 
    IR_READ, 
    IR_WRITE 
} IRKind;
typedef enum {
    RELOP_EQ,   // == 
    RELOP_LT,   // <
    RELOP_GT,   // >
    RELOP_LE,   // <=
    RELOP_GE,   // >=
    RELOP_NE    // !=
} RELOP_t;

struct Operand {
    OperandKind kind;
    union {
        int tmpId;  // for temporary variable
        int labelId;    // for label
        int value;  // for constant
        Symbol *symbol; // for funcion & variable
    } u;
};
struct IR {
    IRKind kind;
    Operand result;
    Operand arg1;
    Operand arg2;
    union {
        RELOP_t relop;    // for relop
        int size;   // for dec operator
    } u;
};
struct IRList {
    IR code;
    IRList *prev;
    IRList *next;
};
struct ArgNode {
    int tmpId;
    ArgNode *next;
};

struct DeadCode {
    IRList *irList;
    DeadCode *next;
};

void generate_ir(Node *root, const char* filename); // save ir code to file or print to screen
IRList* getCodeList();
void clearIRList();
bool isOperandEqual(Operand op1, Operand op2);
#endif
