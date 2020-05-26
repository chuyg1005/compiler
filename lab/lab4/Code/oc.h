#ifndef __OC_H__
#define __OC_H__
#include "ir.h"
#define REG_NAME_LEN 8

typedef struct LocalVar LocalVar;
typedef struct Reg Reg;
typedef struct LVList LVList;

struct LocalVar {
    Operand op;
    int off;
};
struct Reg {
    char name[REG_NAME_LEN];
    bool used;
    bool modified;
    bool locked;
    LocalVar *var;
};
struct LVList {
    LocalVar var;
    LVList *next;
};

void generate_oc(IRList *codeList, const char *filename);
#endif
