#include "ir.h"
#include "syntax.tab.h"
#include "hash_table.h"
#include <assert.h>

#define LABEL_FALL 0
#define VAR_NULL 0

static IRList* codeList = NULL;
static FILE* stream = NULL;
static bool illegal = false;
static void addCode(IRList *code); // add code to the end of codeList
static void initIRList();
/* static void clearIRList();  // dealloc irlist */
static void translate(Node *node);  // entry
static void removeCode(IRList *code);

static int newLableId();
static int newTmpId();

static void translateExtDef(Node *extDef);
static void translateDec(Node *dec);
static void translateStmt(Node *stmt);

static void translateFunDec(Node *funDec); // param 
static void translateExp(Node *exp, int place);
static void translateArr(Node *exp, int place);
static void translateCond(Node *exp, int label_true, int label_false);
static ArgNode* translateArgs(ArgNode *argList, Node *args);
static ArgNode* translateArgs1(Node *args);

static void printOperand(Operand op);
static void printRelop(RELOP_t relop);
static void printCodeList(); 

static void genGoto(int labelId);
static void genLabel(int labelId);
static RELOP_t getRelop(Node *relop);
static RELOP_t getRevRelop(RELOP_t relop);
static int getTypeSize(Type *type);
static IRList* newIRList();

static void optimize();
static void optimize_once(bool *changed);
static void last_optimize();
static void substitute();
static void assignSubs(bool *changed);
static void assignSubs1(bool *changed);
static void evalConst(bool *changed);
static void assignElimit(bool *changed);
static void labelElimit(bool *changed);
static IRList *lookback(IRList *list, IRList *p);
static DeadCode* updateDeadList(DeadCode *deadList, Operand op);

static bool isOperandValid(Operand Operand);
static bool isModifyInstr(IRList *irList, Operand op);
static bool checkOrder(IRList *p1, IRList *p2, IRList *end);

void generate_ir(Node *root, const char* filename) {
    initIRList();
    translate(root);
    /* printf("=====================before optimize===================\n"); */
    /* printCodeList(); */
    /* printf("=====================after optimize===================\n"); */
#ifdef __ARR__
    illegal = false;
#endif
    if(!illegal) {
#ifdef __OPT__
        optimize();
#endif
#ifndef __LAB4__
        stream = fopen(filename, "w");
        if(!stream) {
            perror("fopen");
            return;
        }
        printCodeList();
        fclose(stream);
#endif
    } else {
        fprintf(stderr, "\033[31mCannot translate: Code contains variables of multi-dimensional array type or parameters of array type[use -D__ARR__ to change the behavior of compiler]\n\033[0m");
    }
    // do something
#ifndef __LAB4__
    clearIRList();
    clearSymbolTable();
#endif
}

IRList* getCodeList() {
    return codeList;
}

void printOperand(Operand op) {
    switch(op.kind) {
        case OP_VAR:
            fprintf(stream, "%s", op.u.symbol->name);
            break;
        case OP_FUNC:
            fprintf(stream, "%s", op.u.symbol->name);
            break;
        case OP_TEMP:
            fprintf(stream, "t_%d", op.u.tmpId);
            break;
        case OP_CONST:
            fprintf(stream, "#%d", op.u.value);
            break;
        case OP_LABEL:
            fprintf(stream, "label_%d", op.u.labelId);
            break;
        default:
            assert(0);
    }
}

void printRelop(RELOP_t relop) {
    switch(relop) {
        case RELOP_EQ:
            fprintf(stream, " == ");
            break;
        case RELOP_LE:
            fprintf(stream, " <= ");
            break;
        case RELOP_LT:
            fprintf(stream, " < ");
            break;
        case RELOP_GE:
            fprintf(stream, " >= ");
            break;
        case RELOP_GT:
            fprintf(stream, " > ");
            break;
        case RELOP_NE:
            fprintf(stream, " != ");
            break;
        default:
            assert(0);
    }
}

void printCodeList() {
    if(codeList == NULL) return;
    IRList *p = codeList;
    do {
        switch(p->code.kind) {
            case IR_LABEL:
                fprintf(stream, "LABEL ");
                printOperand(p->code.arg1);
                fprintf(stream, " :\n");
                break;
            case IR_FUNC:
                fprintf(stream, "FUNCTION ");
                printOperand(p->code.arg1);
                fprintf(stream, " :\n");
                break;
            case IR_ASSIGN:
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_ADD:
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, " + ");
                printOperand(p->code.arg2);
                fprintf(stream, "\n");
                break;
            case IR_SUB:
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, " - ");
                printOperand(p->code.arg2);
                fprintf(stream, "\n");
                break;
            case IR_MUL:
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, " * ");
                printOperand(p->code.arg2);
                fprintf(stream, "\n");
                break;
            case IR_DIV:
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, " / ");
                printOperand(p->code.arg2);
                fprintf(stream, "\n");
                break;
            case IR_REF:
                printOperand(p->code.result);
                fprintf(stream, " := &");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_DEREF_R:
                printOperand(p->code.result);
                fprintf(stream, " := *");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_DEREF_L:
                fprintf(stream, "*");
                printOperand(p->code.result);
                fprintf(stream, " := ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_GOTO:
                fprintf(stream, "GOTO ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_RELOP:
                fprintf(stream, "IF ");
                printOperand(p->code.arg1);
                printRelop(p->code.u.relop);
                printOperand(p->code.arg2);
                fprintf(stream, " GOTO ");
                printOperand(p->code.result);
                fprintf(stream, "\n");
                break;
            case IR_RET:
                fprintf(stream, "RETURN ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_DEC:
                fprintf(stream, "DEC ");
                printOperand(p->code.result);
                fprintf(stream, " ");
                fprintf(stream, "%d", p->code.arg1.u.value);
                /* printOperand(p->code.arg1); */
                fprintf(stream, "\n");
                break;
            case IR_ARG:
                fprintf(stream, "ARG ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_CALL:
                printOperand(p->code.result);
                fprintf(stream, " := CALL ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_PARM:
                fprintf(stream, "PARAM ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_READ:
                fprintf(stream, "READ ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            case IR_WRITE:
                fprintf(stream, "WRITE ");
                printOperand(p->code.arg1);
                fprintf(stream, "\n");
                break;
            default:
                assert(0);
        } 
        p = p->next;
    } while(p != codeList);
}

void genGoto(int labelId) {
    IRList *irList = newIRList();
    irList->code.kind = IR_GOTO;
    irList->code.arg1.kind = OP_LABEL;
    irList->code.arg1.u.labelId = labelId;
    addCode(irList);
}

void genLabel(int labelId) {
    IRList *irList = newIRList();
    irList->code.kind = IR_LABEL;
    irList->code.arg1.kind = OP_LABEL;
    irList->code.arg1.u.labelId = labelId;
    addCode(irList);
}

void initIRList() {
    /* create list head node */
}

// bidirect linkedlist insert
void addCode(IRList *code) {
    if(code) {
        if(!codeList) {
            codeList = code;
        } else {
            code->prev = codeList->prev;
            codeList->prev->next = code;
            code->next = codeList;
            codeList->prev = code;
        }
    }
}

void clearIRList() {
    if(!codeList) return;
    IRList *end = codeList;
    while(codeList->next != end) {
        IRList *p = codeList;
        codeList = p->next;
        free(p);
    }
    free(codeList);
}

static int newLableId() {
    static int labelId = 1;
    return labelId++;
}

static int newTmpId() {
    static int tmpId = 1;
    return tmpId++;
}

RELOP_t getRelop(Node *relop) {
    assert(relop->type == NODE_RELOP);
    if(strcmp(relop->val.name, "<") == 0) {
        return RELOP_LT;
    } else if(strcmp(relop->val.name, "<=") == 0) {
        return RELOP_LE;
    } else if(strcmp(relop->val.name, ">") == 0) {
        return RELOP_GT;
    } else if(strcmp(relop->val.name, ">=") == 0) {
        return RELOP_GE;
    } else if(strcmp(relop->val.name, "==") == 0) {
        return RELOP_EQ;
    } else if(strcmp(relop->val.name, "!=") == 0) {
        return RELOP_NE;
    } else {
        assert(0);
    }
}

RELOP_t getRevRelop(RELOP_t relop) {
    switch(relop) {
        case RELOP_EQ:
            return RELOP_NE;
        case RELOP_LE:
            return RELOP_GT;
        case RELOP_LT:
            return RELOP_GE;
        case RELOP_GE:
            return RELOP_LT;
        case RELOP_GT:
            return RELOP_LE;
        case RELOP_NE:
            return RELOP_EQ;
        default:
            assert(0);
    }
}

IRList* newIRList() {
    IRList *irList = (IRList*)malloc(sizeof(IRList));
    memset(irList, 0, sizeof(IRList));
    irList->prev = irList->next = irList;
    /* irList->prev = NULL; */
    /* irList->next = NULL; */
    return irList;
}

void translate(Node *node) {
    if(!node) return;
    switch(node->type) {
        case NODE_ExtDef:
            translateExtDef(node);
            return;
        case NODE_Dec:
            translateDec(node);
            return;
        case NODE_Stmt:
            translateStmt(node);
            return;
        default:
            break;
    }
    for(Node *p = node->child; p; p=p->sib) {
        translate(p);
    }
}

void translateExtDef(Node *extDef) {
    assert(extDef->type == NODE_ExtDef);
    // extdef -> specifier fundec compst
    if(extDef->child->sib->type == NODE_FunDec) {
        translateFunDec(extDef->child->sib);
        translate(extDef->child->sib->sib); // translate compst
    }
    // no global variables, no need to deal it
}

void translateFunDec(Node *funDec) {
    assert(funDec->type == NODE_FunDec);
    // generate funcion
    IRList *irList = newIRList();
    irList->code.kind = IR_FUNC;
    irList->code.arg1.kind = OP_FUNC;
    Symbol *sym = lookupSymbol(funDec->child->val.name, SYM_FUNC);
    assert(sym);
    irList->code.arg1.u.symbol = sym;
    addCode(irList);
    // generate parameter declare
    if(funDec->childno == 4) {  // fundec -> id lp varlist rp
        Node *varList = funDec->child->sib->sib;
        assert(varList->type == NODE_VarList);
        while(true) {
            Node *paramDec = varList->child;
            Node *varDec = paramDec->child->sib;
            while(varDec->childno != 1) {
                varDec = varDec->child;
            }

            IRList *irList = newIRList();
            irList->code.kind = IR_PARM;
            irList->code.arg1.kind = OP_VAR;
            Symbol *sym = lookupSymbol(varDec->child->val.name, SYM_VAR);
            assert(sym);
            if(sym->u.type->kind == ARRAY) {
                illegal = true;
            }
            irList->code.arg1.u.symbol = sym;
            addCode(irList);

            if(varList->childno == 1) break;
            varList = varList->child->sib->sib;            
       }
    }
}

void translateDec(Node *dec) {
    assert(dec->type == NODE_Dec);
    Node *varDec = dec->child;
    while(varDec->childno != 1) {
        varDec = varDec->child;
    }
    Symbol *sym = lookupSymbol(varDec->child->val.name, SYM_VAR);
    assert(sym);
    int size = getTypeSize(sym->u.type);
    if(dec->childno == 1) { // dec -> vardec
        if(sym->u.type->kind != BASIC) {    // use dec to allocate mem
            if(sym->u.type->kind == ARRAY) {
                illegal = true;
            }
        /* if(size > 4) { */
            int t1 = newTmpId();
            IRList *irList = newIRList();
            irList->code.kind = IR_DEC;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = t1;
            irList->code.arg1.kind = OP_CONST;
            irList->code.arg1.u.value = size;
            addCode(irList);

            irList = newIRList();
            irList->code.kind = IR_REF;
            irList->code.result.kind = OP_VAR;
            irList->code.result.u.symbol = sym;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = t1;
            addCode(irList);
        }
    } else {    // only basic variable is allow to initial
        assert(dec->child->child->type == NODE_ID);
        int t1 = newTmpId();
        translateExp(dec->child->sib->sib, t1);

        IRList *irList = newIRList();
        irList->code.kind = IR_ASSIGN;
        irList->code.result.kind = OP_VAR;
        irList->code.result.u.symbol = sym;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;

        addCode(irList);
    }
}

int getTypeSize(Type *type) {
    if(type->kind == BASIC) {
        return 4;
    }
    if(type->kind == ARRAY) {
        return type->u.array.size * getTypeSize(type->u.array.elem);
    }
    assert(type->kind == STRUCTURE);
    int size = 0;
    FieldList *field = type->u.structure;
    while(field) {
        size += getTypeSize(field->type);
        field = field->next;
    }
    return size;
}

// translate array
void translateArr(Node *exp, int place) {
    assert(exp->child->sib->type == NODE_LB);
    // Exp -> exp1 LB exp2 RB
    Node *exp1 = exp->child;
    Node *exp2 = exp1->sib->sib;
    int index = newTmpId();
    translateExp(exp2, index);
    Type *type = getExpType(exp);
    int size = getTypeSize(type);
    int offset = newTmpId();

    // offset = index * size
    IRList *irList = newIRList();
    irList->code.kind = IR_MUL;
    irList->code.result.kind = OP_TEMP;
    irList->code.result.u.tmpId = offset;
    irList->code.arg1.kind = OP_TEMP;
    irList->code.arg1.u.tmpId = index;
    irList->code.arg2.kind = OP_CONST;
    irList->code.arg2.u.value = size;
    addCode(irList);

    if(type->kind != BASIC) {
        int t1 = newTmpId();
        translateExp(exp1, t1);
        irList = newIRList();
        irList->code.kind = IR_ADD;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = offset;
        addCode(irList);
        return;
    }

    int t1 = newTmpId();
    translateExp(exp1, t1);
    int addr = newTmpId();
    irList = newIRList();
    irList->code.kind = IR_ADD;
    irList->code.result.kind = OP_TEMP;
    irList->code.result.u.tmpId = addr;
    irList->code.arg1.kind = OP_TEMP;
    irList->code.arg1.u.tmpId = t1;
    irList->code.arg2.kind = OP_TEMP;
    irList->code.arg2.u.tmpId = offset;
    addCode(irList);

    irList = newIRList();
    irList->code.kind = IR_DEREF_R;
    irList->code.result.kind = OP_TEMP;
    irList->code.result.u.tmpId = place;
    irList->code.arg1.kind = OP_TEMP;
    irList->code.arg1.u.tmpId = addr;
    addCode(irList);


    /* // exp -> exp lb exp rb */
    /* // exp -> exp dot exp */
    /* if(exp1->child->type == NODE_ID) { */
    /*     Symbol *sym = lookupSymbol(exp1->child->val.name, SYM_VAR); */
    /*     assert(sym); */

    /*     irList = newIRList(); */
    /*     irList->code.kind = IR_ADD; */
    /*     irList->code.result.kind = OP_TEMP; */
    /*     irList->code.result.u.tmpId = place; */
    /*     irList->code.arg1.kind = OP_VAR; */
    /*     irList->code.arg1.u.symbol = sym; */
    /*     irList->code.arg2.kind = OP_TEMP; */
    /*     irList->code.arg2.u.tmpId = offset; */
    /*     addCode(irList); */
    /* } else if(exp1->child->sib->type == NODE_LB){ */
    /*     assert(exp1->child->sib->type == NODE_LB); */

    /*     int t1 = newTmpId(); */
    /*     translateExp(exp1, t1); */
    /*     /1* translateArr(exp1, t1); *1/ */
    /*     irList = newIRList(); */
    /*     irList->code.kind = IR_ADD; */
    /*     irList->code.result.kind = OP_TEMP; */
    /*     irList->code.result.u.tmpId = place; */
    /*     irList->code.arg1.kind = OP_TEMP; */
    /*     irList->code.arg1.u.tmpId = t1; */
    /*     irList->code.arg2.kind = OP_TEMP; */
    /*     irList->code.arg2.u.tmpId = offset; */
    /*     addCode(irList); */
    /* } else if(exp1->child->sib->type == NODE_DOT) { */
    /*     int addr = newTmpId(); */
    /*     translateExp(exp1->child, addr); */
    /*     Type *type = getExpType(exp1->child); */
    /*     assert(type->kind == STRUCTURE); */
    /*     char *name = exp1->child->sib->sib->val.name; */
    /*     FieldList *field = type->u.structure; */
    /*     int offset1 = 0; */
    /*     while(strcmp(field->name, name) != 0) { */
    /*         offset1 += getTypeSize(field->type); */
    /*         assert(field->next); */
    /*         field = field->next; */
    /*     } */
        
    /*     int t1 = newTmpId(); */
    /*     irList = newIRList(); */
    /*     irList->code.kind = IR_ADD; */
    /*     irList->code.result.kind = OP_TEMP; */
    /*     irList->code.result.u.tmpId = t1; */
    /*     irList->code.arg1.kind = OP_TEMP; */
    /*     irList->code.arg1.u.tmpId = addr; */
    /*     irList->code.arg2.kind = OP_CONST; */
    /*     irList->code.arg2.u.value = offset1; */
    /*     addCode(irList); */

    /*     irList = newIRList(); */
    /*     irList->code.kind = IR_ADD; */
    /*     irList->code.result.kind = OP_TEMP; */
    /*     irList->code.result.u.tmpId = place; */
    /*     irList->code.arg1.kind = OP_TEMP; */
    /*     irList->code.arg1.u.tmpId = t1; */
    /*     irList->code.arg2.kind = OP_TEMP; */
    /*     irList->code.arg2.u.value = offset; */
    /*     addCode(irList); */
        
    /*     /1* assert(0);  // TOOD *1/ */
    /* } */
}

void translateExp(Node *exp, int place) {
    assert(exp->type == NODE_Exp);
    if(exp->childno == 1) {
        IRList *irList = newIRList();
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        if(exp->child->type == NODE_INT) {
            irList->code.kind = IR_ASSIGN;
            irList->code.arg1.kind = OP_CONST;
            irList->code.arg1.u.value = exp->child->val.intVal;
            addCode(irList);
        } else if(exp->child->type == NODE_ID) {
            Symbol *sym = lookupSymbol(exp->child->val.name, SYM_VAR);
            assert(sym);
            irList->code.kind = IR_ASSIGN;
            irList->code.arg1.kind = OP_VAR;
            irList->code.arg1.u.symbol = sym;
            addCode(irList);
        } else {
            assert(0);
        }
        return;
    }
    if(exp->child->type == NODE_LP) {   // parenthese
        translateExp(exp->child->sib, place);
        return;
    } else if(exp->child->type == NODE_ID) {    // function call
        Symbol *sym = lookupSymbol(exp->child->val.name, SYM_FUNC);
        assert(sym);
        if(exp->childno == 3) {
            IRList *irList = newIRList();
            if(strcmp(sym->name, "read") == 0) {
                irList->code.kind = IR_READ;
                irList->code.arg1.kind = OP_TEMP;
                irList->code.arg1.u.tmpId = place;
            } else {
                irList->code.kind = IR_CALL;
                irList->code.result.kind = OP_TEMP;
                irList->code.result.u.tmpId = place;
                irList->code.arg1.kind = OP_FUNC;
                irList->code.arg1.u.symbol = sym;
            }
            addCode(irList);
        } else if(exp->childno == 4) {
            ArgNode *argList = NULL;
            argList = translateArgs1(exp->child->sib->sib);
            /* argList = translateArgs(argList, exp->child->sib->sib); */
            if(strcmp(sym->name, "write") == 0) {
                assert(argList->next == NULL);
                IRList *irList = newIRList(); 
                irList->code.kind = IR_WRITE;
                irList->code.arg1.kind = OP_TEMP;
                irList->code.arg1.u.tmpId = argList->tmpId;
                addCode(irList);
            } else {
                ArgNode *p = argList;
                IRList *irList = NULL;
                while(p) {
                    irList = newIRList();
                    irList->code.kind = IR_ARG;
                    irList->code.arg1.kind = OP_TEMP;
                    irList->code.arg1.u.tmpId = p->tmpId;
                    addCode(irList);
                    p = p->next;
                }
                irList = newIRList();
                irList->code.kind = IR_CALL;
                irList->code.result.kind = OP_TEMP;
                irList->code.result.u.tmpId = place;
                irList->code.arg1.kind = OP_FUNC;
                irList->code.arg1.u.symbol = sym;
                addCode(irList);
            }

            // deallocate arglist
            while(argList) {
                ArgNode *p = argList;
                argList = p->next;
                free(p);
            }
        } else {
            assert(0);
        }
        return;
    }
    if(exp->child->sib->type == NODE_ASSIGNOP) {
        Node *exp1 = exp->child;
        Node *exp2 = exp1->sib->sib;
        int rvalue = newTmpId();    // t1 is result of rvalue
        translateExp(exp2, rvalue);

        IRList *irList = newIRList();
        irList->code.kind = IR_ASSIGN;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = rvalue;
        addCode(irList);

        if(exp1->child->type == NODE_ID) {
            Symbol *sym = lookupSymbol(exp1->child->val.name, SYM_VAR);
            assert(sym);

            // assign to variable
            irList = newIRList();
            irList->code.kind = IR_ASSIGN;
            irList->code.result.kind = OP_VAR;
            irList->code.result.u.symbol = sym;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = rvalue;
            addCode(irList);
        } else if(exp1->child->sib->type == NODE_LB) {    // array
            int index = newTmpId();
            translateExp(exp1->child->sib->sib, index);
            Type *type = getExpType(exp1);
            int size = getTypeSize(type);
            int offset = newTmpId();
            // offset = index * size
            IRList *irList = newIRList();
            irList->code.kind = IR_MUL;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = offset;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = index;
            irList->code.arg2.kind = OP_CONST;
            irList->code.arg2.u.value = size;
            addCode(irList);
            int t1 = newTmpId();
            translateExp(exp1->child, t1);
            int addr = newTmpId();
            irList = newIRList();
            irList->code.kind = IR_ADD;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = addr;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = t1;
            irList->code.arg2.kind = OP_TEMP;
            irList->code.arg2.u.value = offset;
            addCode(irList);
            /* translateArr(exp1, addr); */

            irList = newIRList();
            irList->code.kind = IR_DEREF_L;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = addr;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = rvalue;
            addCode(irList);
        } else if(exp1->child->sib->type == NODE_DOT) { // structure
            int addr = newTmpId();
            translateExp(exp1->child, addr);
            Type *type = getExpType(exp1->child);
            assert(type->kind == STRUCTURE);
            char *name = exp1->child->sib->sib->val.name;
            FieldList *field = type->u.structure;
            int offset = 0;
            while(strcmp(field->name, name) != 0) {
                offset += getTypeSize(field->type);
                assert(field->next);
                field = field->next;
            }
            
            int t1 = newTmpId();
            irList = newIRList();
            irList->code.kind = IR_ADD;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = t1;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = addr;
            irList->code.arg2.kind = OP_CONST;
            irList->code.arg2.u.value = offset;
            addCode(irList);

            irList = newIRList();
            irList->code.kind = IR_DEREF_L;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = t1;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = rvalue;
            addCode(irList);
            /* assert(0);  // todo */
        } else {
            assert(0);
        }
    } else if(exp->child->sib->type == NODE_PLUS) {
        int t1 = newTmpId();
        int t2 = newTmpId();
        translateExp(exp->child, t1);
        translateExp(exp->child->sib->sib, t2);

        IRList *irList = newIRList();
        irList->code.kind = IR_ADD;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = t2;
        addCode(irList);
    } else if(exp->child->sib->type == NODE_MINUS) {
        int t1 = newTmpId();
        int t2 = newTmpId();
        translateExp(exp->child, t1);
        translateExp(exp->child->sib->sib, t2);

        IRList *irList = newIRList();
        irList->code.kind = IR_SUB;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = t2;
        addCode(irList);
    } else if(exp->child->sib->type == NODE_STAR) {
        int t1 = newTmpId();
        int t2 = newTmpId();
        translateExp(exp->child, t1);
        translateExp(exp->child->sib->sib, t2);

        IRList *irList = newIRList();
        irList->code.kind = IR_MUL;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = t2;
        addCode(irList);
    } else if(exp->child->sib->type == NODE_DIV) {
        int t1 = newTmpId();
        int t2 = newTmpId();
        translateExp(exp->child, t1);
        translateExp(exp->child->sib->sib, t2);

        IRList *irList = newIRList();
        irList->code.kind = IR_DIV;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = t2;
        addCode(irList);
    } else if(exp->child->type == NODE_MINUS) {
        int t1 = newTmpId();
        translateExp(exp->child->sib, t1);

        IRList *irList = newIRList();
        irList->code.kind = IR_SUB;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_CONST;
        irList->code.arg1.u.value = 0;
        irList->code.arg2.kind = OP_TEMP;
        irList->code.arg2.u.tmpId = t1;
        addCode(irList);
    } else if(exp->child->sib->type == NODE_RELOP
            || exp->child->sib->type == NODE_AND
            || exp->child->sib->type == NODE_OR
            || exp->child->type == NODE_NOT) {
        int label_true = newLableId();
        int label_false = newLableId();

        // pre assign 0
        IRList *irList = newIRList();
        irList->code.kind = IR_ASSIGN;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_CONST;
        irList->code.arg1.u.value = 0;
        addCode(irList);

        translateCond(exp, label_true, label_false);
        genLabel(label_true);

        irList = newIRList();
        irList->code.kind = IR_ASSIGN;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_CONST;
        irList->code.arg1.u.value = 1;
        addCode(irList);

        genLabel(label_false);
    } else if(exp->child->sib->type == NODE_LB) {   // exp -> exp lb exp rb
        translateArr(exp, place);
        /* int addr = newTmpId(); */
        /* translateArr(exp, addr); */

        /* IRList *irList = newIRList(); */
        /* irList->code.kind = IR_DEREF_R; */
        /* irList->code.result.kind = OP_TEMP; */
        /* irList->code.result.u.tmpId = place; */
        /* irList->code.arg1.kind = OP_TEMP; */
        /* irList->code.arg1.u.tmpId = addr; */
        /* addCode(irList); */
    } else if(exp->child->sib->type == NODE_DOT) {
        int addr = newTmpId();
        translateExp(exp->child, addr);
        Type *type = getExpType(exp->child);
        assert(type->kind == STRUCTURE);
        char *name = exp->child->sib->sib->val.name;
        FieldList *field = type->u.structure;
        int offset = 0;
        while(strcmp(field->name, name) != 0) {
            offset += getTypeSize(field->type);
            assert(field->next);
            field = field->next;
        }

        Type *type1 = getExpType(exp);
        if(type1->kind != BASIC) {
            IRList *irList = newIRList();
            irList->code.kind = IR_ADD;
            irList->code.result.kind = OP_TEMP;
            irList->code.result.u.tmpId = place;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = addr;
            irList->code.arg2.kind = OP_CONST;
            irList->code.arg2.u.value = offset;
            addCode(irList);
            return;
        }
        
        int t1 = newTmpId();
        IRList *irList = newIRList();
        irList->code.kind = IR_ADD;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = t1;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = addr;
        irList->code.arg2.kind = OP_CONST;
        irList->code.arg2.u.value = offset;
        addCode(irList);

        irList = newIRList();
        irList->code.kind = IR_DEREF_R;
        irList->code.result.kind = OP_TEMP;
        irList->code.result.u.tmpId = place;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        addCode(irList);
    } else {
        assert(0);
    }
}

void translateCond(Node *exp, int label_true, int label_false) {
    if(exp->childno == 1) { // id and int
        int t1 = newTmpId();
        translateExp(exp, t1);

        IRList *irList = newIRList();
        irList->code.kind = IR_RELOP;
        irList->code.result.kind = OP_LABEL;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_CONST;
        irList->code.arg2.u.value = 0;
        if(label_true != LABEL_FALL && label_false != LABEL_FALL) {
            irList->code.result.u.labelId = label_true;
            irList->code.u.relop = RELOP_NE;
            addCode(irList);
            genGoto(label_false);
        } else if(label_true == LABEL_FALL) {
            irList->code.result.u.labelId = label_false;
            irList->code.u.relop = RELOP_EQ;
            addCode(irList);
        } else if(label_false == LABEL_FALL) {
            irList->code.result.u.labelId = label_true;
            irList->code.u.relop = RELOP_NE;
            addCode(irList);
        }
        return;
    }
    if(exp->child->type == NODE_NOT) {
        translateCond(exp->child->sib, label_false, label_true);
    } else if(exp->child->sib->type == NODE_RELOP) {
        int t1 = newTmpId();
        int t2 = newTmpId();
        translateExp(exp->child, t1);
        translateExp(exp->child->sib->sib, t2);

        if(label_true != LABEL_FALL && label_false != LABEL_FALL) {
            IRList *irList = newIRList();
            irList->code.kind = IR_RELOP;
            irList->code.u.relop = getRelop(exp->child->sib);
            irList->code.result.kind = OP_LABEL;
            irList->code.result.u.labelId = label_true;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = t1;
            irList->code.arg2.kind = OP_TEMP;
            irList->code.arg2.u.tmpId = t2;
            addCode(irList);
            genGoto(label_false);
        } else if(label_true == LABEL_FALL) {
            IRList *irList = newIRList();
            irList->code.kind = IR_RELOP;
            irList->code.u.relop = getRevRelop(getRelop(exp->child->sib));
            irList->code.result.kind = OP_LABEL;
            irList->code.result.u.labelId = label_false;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = t1;
            irList->code.arg2.kind = OP_TEMP;
            irList->code.arg2.u.tmpId = t2;
            addCode(irList);
        } else if(label_false == LABEL_FALL) {
            IRList *irList = newIRList();
            irList->code.kind = IR_RELOP;
            irList->code.u.relop = getRelop(exp->child->sib);
            irList->code.result.kind = OP_LABEL;
            irList->code.result.u.labelId = label_true;
            irList->code.arg1.kind = OP_TEMP;
            irList->code.arg1.u.tmpId = t1;
            irList->code.arg2.kind = OP_TEMP;
            irList->code.arg2.u.tmpId = t2;
            addCode(irList);
        } else {
            assert(0);
        }
    } else if(exp->child->sib->type == NODE_AND) {
        if(label_false == LABEL_FALL) {
            int exp1_false = newLableId();
            translateCond(exp->child, LABEL_FALL, exp1_false);
            translateCond(exp->child->sib->sib, label_true, label_false);
            genLabel(exp1_false);
        } else {
            translateCond(exp->child, LABEL_FALL, label_false);
            translateCond(exp->child->sib->sib, label_true, label_false);
        }
    } else if(exp->child->sib->type == NODE_OR) {
        if(label_true == LABEL_FALL) {
            int exp1_true = newLableId();
            translateCond(exp->child, exp1_true, LABEL_FALL);
            translateCond(exp->child->sib->sib, label_true, label_false);
            genLabel(exp1_true);
        } else {
            translateCond(exp->child, label_true, LABEL_FALL);
            translateCond(exp->child->sib->sib, label_true, label_false);
        }
    } else if(exp->child->type == NODE_LP){
        translateCond(exp->child->sib, label_true, label_false);
    } else {
        int t1 = newTmpId();
        translateExp(exp, t1);

        IRList *irList = newIRList();
        irList->code.kind = IR_RELOP;
        irList->code.result.kind = OP_LABEL;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        irList->code.arg2.kind = OP_CONST;
        irList->code.arg2.u.value = 0;
        if(label_true != LABEL_FALL && label_false != LABEL_FALL) {
            irList->code.result.u.labelId = label_true;
            irList->code.u.relop = RELOP_NE;
            addCode(irList);
            genGoto(label_false);
        } else if(label_true == LABEL_FALL) {
            irList->code.result.u.labelId = label_false;
            irList->code.u.relop = RELOP_EQ;
            addCode(irList);
        } else if(label_false == LABEL_FALL) {
            irList->code.result.u.labelId = label_true;
            irList->code.u.relop = RELOP_NE;
            addCode(irList);
        } else {
            assert(0);
        }
    }
}

ArgNode* translateArgs(ArgNode *argList, Node *args) {
    assert(args->type == NODE_Args);

    int t1 = newTmpId();
    translateExp(args->child, t1);
    ArgNode *arg = (ArgNode*)malloc(sizeof(ArgNode));
    arg->next = argList;
    arg->tmpId = t1;

    if(args->childno == 1) {
        return arg;
    } else {
        return translateArgs(arg, args->child->sib->sib);
    }
}

ArgNode* translateArgs1(Node *args) {
    assert(args->type == NODE_Args);
    if(args->childno == 1) {
        int t1 = newTmpId();
        translateExp(args->child, t1);
        ArgNode *arg = (ArgNode*)malloc(sizeof(ArgNode));
        arg->next = NULL;
        arg->tmpId = t1;
        return arg;
    } else {
        ArgNode* argList = translateArgs1(args->child->sib->sib);
        int t1 = newTmpId();
        translateExp(args->child, t1);
        ArgNode *arg = (ArgNode*)malloc(sizeof(ArgNode));
        arg->next = NULL;
        arg->tmpId = t1;
        ArgNode *p = argList;
        while(p->next) {
            p = p->next;
        }
        p->next = arg;
        return argList;
    }
}

void translateStmt(Node *stmt) {
    assert(stmt->type == NODE_Stmt);
    if(stmt->child->type == NODE_Exp) {
        translateExp(stmt->child, VAR_NULL);
    } else if(stmt->child->type == NODE_CompSt) {
        translate(stmt->child);
    } else if(stmt->child->type == NODE_RETURN) {
        int t1 = newTmpId();
        translateExp(stmt->child->sib, t1);
        IRList *irList = newIRList();
        irList->code.kind = IR_RET;
        irList->code.arg1.kind = OP_TEMP;
        irList->code.arg1.u.tmpId = t1;
        addCode(irList);
    } else if(stmt->child->type == NODE_WHILE) {
        int begin = newLableId();
        int label_false = newLableId();

        genLabel(begin);
        translateCond(stmt->child->sib->sib, LABEL_FALL, label_false);
        translateStmt(stmt->child->sib->sib->sib->sib);
        genGoto(begin);
        genLabel(label_false);
    } else if(stmt->child->type == NODE_IF) {
        if(stmt->childno == 5) {  // if lb exp rb stmt
            int label_false = newLableId();
            translateCond(stmt->child->sib->sib, LABEL_FALL, label_false);
            translateStmt(stmt->child->sib->sib->sib->sib);
            genLabel(label_false);
        } else if(stmt->childno == 7) {
            int label_false = newLableId();
            int label_next = newLableId();
            translateCond(stmt->child->sib->sib, LABEL_FALL, label_false);
            translateStmt(stmt->child->sib->sib->sib->sib);
            genGoto(label_next);
            genLabel(label_false);
            translateStmt(stmt->child->sib->sib->sib->sib->sib->sib);
            genLabel(label_next);
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
}

bool isOperandValid(Operand operand) {
    return operand.kind != OP_INV;
}

void optimize() {
    bool changed = false;
    do {
        /* printf("optimize once\n"); */
        optimize_once(&changed);
    } while(changed);
    /* last_optimize(); */
}

void removeCode(IRList *code) {
    // single node
    if(code->next == code) {
        codeList = NULL;
        free(code);
        return;
    }
    // pick up code node
    code->next->prev = code->prev;
    code->prev->next = code->next;
    if(code == codeList) {  // change global codelist
        codeList = code->next;
    }
    free(code);
}

void optimize_once(bool *changed) {
    *changed = false;
    if(!codeList) return;
    // assignment optimization
    assignSubs(changed);
    // constant optimization
    evalConst(changed);
    // delete unused code
    assignElimit(changed);

    labelElimit(changed);
}

// remove all dead code which is related to op
DeadCode* updateDeadList(DeadCode *deadList, Operand op) {
    DeadCode *cur = deadList;
    DeadCode *prev = deadList;
    while(cur) {
        if(isOperandEqual(cur->irList->code.result, op)) { // remove cur node
            DeadCode *p = cur;
            if(cur == deadList) {
                deadList = cur->next;
                prev = deadList;
                cur = deadList;
            } else {
                prev->next = cur->next;
                cur = cur->next;
            }
            free(p);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    return deadList;
}

bool isOperandEqual(Operand op1, Operand op2) {
    if(op1.kind == op2.kind) {
        if(op1.kind == OP_TEMP) {
            return op1.u.tmpId == op2.u.tmpId;
        } else if(op1.kind == OP_VAR) {
            return op1.u.symbol == op2.u.symbol;
        } else if(op1.kind == OP_CONST) {
            return op1.u.value == op2.u.value;
        }
    }
    return false;
}

void assignSubs1(bool *changed) {
    IRList *p = codeList;
    IRList *irList1 = NULL;
    IRList *irList2 = NULL;
    HashTable *hashTable = newHashTable();
    do {
        switch(p->code.kind) {
            case IR_LABEL:
            case IR_FUNC:
                HT_clear(hashTable);
                break;
            case IR_ASSIGN:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && !isOperandEqual(irList1->code.result, p->code.result)) {   // TODO: check correctness
                    switch(irList1->code.kind) {
                        case IR_REF:
                            *changed = true;
                            p->code.kind = IR_REF;
                            p->code.arg1 = irList1->code.arg1;
                            break;
                        case IR_DEREF_R:    // warning!!!
                            break;
                        case IR_ASSIGN:
                            if(irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) {
                                *changed = true;
                                p->code.kind = irList1->code.kind;
                                p->code.arg1 = irList1->code.arg1;
                            }
                            break;
                        default:
                            break;
                    }
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.arg2);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1),irList2, p))) {
                    *changed = true;
                    p->code.arg2 = irList2->code.arg1;
                }
                if(isOperandEqual(p->code.arg1, p->code.arg2) && p->code.kind == IR_SUB) {
                    *changed = true;
                    p->code.kind = IR_ASSIGN;
                    p->code.arg1.kind = OP_CONST;
                    p->code.arg1.u.value = 0;
                } else if(isOperandEqual(p->code.arg1, p->code.arg2) && p->code.kind == IR_DIV) {
                    *changed = true;
                    p->code.kind = IR_ASSIGN;
                    p->code.arg1.kind = OP_CONST;
                    p->code.arg1.u.value = 1;
                } else {
                    /* irList1 = lookback(p->prev, p); */
                    /* if(irList1) { */
                    /*     *changed = true; */
                    /*     p->code.kind = IR_ASSIGN; */
                    /*     p->code.arg1 = irList1->code.result; */
                    /* } */
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_REF:
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_DEREF_L:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.result);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p))) {
                    *changed = true;
                    p->code.result = irList2->code.arg1;
                }
                break;
            case IR_DEREF_R:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_ARG:
            case IR_WRITE:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                break;
            /* case IR_GOTO: */
            /*     break; */
            case IR_RELOP:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.arg2);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p))) {
                    *changed = true;
                    p->code.arg2 = irList2->code.arg1;
                }
                /* HT_clear(hashTable); */
                break;
            case IR_RET:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                /* HT_clear(hashTable); */  // TODO: check the correctness
                break;
            /* case IR_DEC: */
            /*     break; */
            case IR_CALL:
                HT_insert(hashTable, p->code.result, p);
                break;
            /* case IR_PARM: */
            /*     break; */
            /* case IR_READ: */
            /*     break; */
            /* case IR_WRITE: */
            /*     break; */
            default:
                break;
        }
        p = p->next;
    } while(p != codeList);
    HT_clear(hashTable);
    free(hashTable);
}

void assignSubs(bool *changed) {
    IRList *p = codeList;
    IRList *irList1 = NULL;
    IRList *irList2 = NULL;
    HashTable *hashTable = newHashTable();
    /* Operand arg1, arg2; */
    do {
        if(isModifyInstr(p->prev, p->code.result) && isModifyInstr(p, p->code.result) && isOperandEqual(p->prev->code.result, p->code.result)) {
            removeCode(p->prev);
        }
        switch(p->code.kind) {
            case IR_LABEL:
            case IR_FUNC:
                HT_clear(hashTable);
                break;
            case IR_ASSIGN:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && !isOperandEqual(irList1->code.result, p->code.result)) {   // TODO: check correctness
                    /* bool flag = false; */
                    switch(irList1->code.kind) {
                        case IR_REF:
                            *changed = true;
                            p->code.kind = IR_REF;
                            p->code.arg1 = irList1->code.arg1;
                            /* flag = true; */
                            break;
                        case IR_DEREF_R:
                            break;
                        case IR_ASSIGN:
                            if(irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) {
                                *changed = true;
                                p->code.kind = irList1->code.kind;
                                p->code.arg1 = irList1->code.arg1;
                            }
                            break;
                        case IR_ADD:
                        case IR_SUB:
                        case IR_MUL:
                        case IR_DIV:
                            if((irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))
                                    && (irList1->code.arg2.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))) {
                                *changed = true;
                                p->code.kind = irList1->code.kind;
                                p->code.arg1 = irList1->code.arg1;
                                p->code.arg2 = irList1->code.arg2;
                            }
                            break;
                        default:
                            break;
                    }
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.arg2);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p))) {
                    *changed = true;
                    p->code.arg2 = irList2->code.arg1;
                }
                if(p->code.arg1.kind != OP_CONST && p->code.arg2.kind == OP_CONST && (p->code.kind == IR_ADD || p->code.kind == IR_SUB)) {
                    if(p->code.kind == IR_SUB) {
                        p->code.kind = IR_ADD;
                        p->code.arg2.u.value = -p->code.arg2.u.value;
                    }
                    Operand arg = p->code.arg1;
                    p->code.arg1 = p->code.arg2;
                    p->code.arg2 = arg;
                }
                if(p->code.kind == IR_ADD) {
                    if(p->code.arg1.kind == OP_CONST && irList2) {  // for example, c = 4 + b
                        if(irList2->code.kind == IR_ADD && irList2->code.arg1.kind == OP_CONST && (irList2->code.arg2.kind == OP_TEMP
                                    || checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) { // b = 4 + a
                            *changed = true;
                            p->code.arg1.u.value += irList2->code.arg1.u.value; 
                            p->code.arg2 = irList2->code.arg2;
                        } else if(irList2->code.kind == IR_SUB && irList2->code.arg1.kind == OP_CONST && (irList2->code.arg2.kind == OP_TEMP
                                    || checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {  // b = 4 - a
                            *changed = true;
                            p->code.arg1.u.value += irList2->code.arg1.u.value;
                            p->code.arg2 = irList2->code.arg2;
                            p->code.kind = IR_SUB;
                        }
                    } else if(irList1 && irList2) { // a + b
                        if(irList1->code.kind == IR_SUB && irList2->code.kind == IR_ADD) {
                            IRList *irList = irList1;
                            irList1 = irList2;
                            irList2 = irList;
                        }
                        if(irList1->code.kind == IR_ADD && irList2->code.kind == IR_SUB)  {
                            if((irList1->code.arg1.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) 
                                    && (irList1->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))
                                    && (irList2->code.arg1.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p)) 
                                    && (irList2->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {
                                if(isOperandEqual(irList1->code.arg1, irList2->code.arg2)) {
                                /* if(irList1->code.arg1.u.tmpId == irList2->code.arg2.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_ADD;
                                    p->code.arg1 = irList1->code.arg2;
                                    p->code.arg2 = irList2->code.arg1;
                                }else if(isOperandEqual(irList1->code.arg2, irList2->code.arg2)) {
                                /* } else if(irList1->code.arg2.u.tmpId == irList2->code.arg2.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_ADD;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg1;
                                }
                            }
                        } else if(irList1->code.kind == IR_SUB && irList2->code.kind == IR_SUB) {
                            if((irList1->code.arg1.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) 
                                    && (irList1->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))
                                    && (irList2->code.arg1.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p)) 
                                    && (irList2->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {
                            /* if(irList1->code.arg1.kind == OP_TEMP && irList1->code.arg2.kind == OP_TEMP */
                            /*         && irList2->code.arg1.kind == OP_TEMP && irList2->code.arg2.kind == OP_TEMP) { */
                                if(isOperandEqual(irList1->code.arg1, irList2->code.arg2)) {
                                /* if(irList1->code.arg1.u.tmpId == irList2->code.arg2.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList2->code.arg1;
                                    p->code.arg2 = irList1->code.arg2;
                                } else if(isOperandEqual(irList1->code.arg2, irList2->code.arg1)) {
                                /* } else if(irList1->code.arg2.u.tmpId == irList2->code.arg1.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg2;
                                }
                            }
                        }
                    }
                } else if(p->code.kind == IR_SUB) {
                    if(p->code.arg1.kind == OP_CONST && irList2) {  // for example, c = 4 - b
                        if(irList2->code.kind == IR_ADD && irList2->code.arg1.kind == OP_CONST 
                                && (irList2->code.arg2.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) { // b = 4 + a
                            *changed = true;
                            p->code.arg1.u.value -= irList2->code.arg1.u.value; 
                            p->code.arg2 = irList2->code.arg2;
                        } else if(irList2->code.kind == IR_SUB && irList2->code.arg1.kind == OP_CONST 
                                && (irList2->code.arg2.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {  // b = 4 - a
                            *changed = true;
                            p->code.arg1.u.value -= irList2->code.arg1.u.value;
                            p->code.arg2 = irList2->code.arg2;
                            p->code.kind = IR_ADD;
                        }
                    } else if(irList1 && irList2) {
                        if(irList1->code.kind == IR_SUB && irList2->code.kind == IR_ADD) {
                            IRList *irList = irList1;
                            irList1 = irList2;
                            irList2 = irList;
                        }
                        if(irList1->code.kind == IR_ADD && irList2->code.kind == IR_SUB)  {
                            if((irList1->code.arg1.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) 
                                    && (irList1->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))
                                    && (irList2->code.arg1.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p)) 
                                    && (irList2->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {
                            /* if(irList1->code.arg1.kind == OP_TEMP && irList1->code.arg2.kind == OP_TEMP */
                            /*         && irList2->code.arg1.kind == OP_TEMP && irList2->code.arg2.kind == OP_TEMP) { */
                                if(isOperandEqual(irList1->code.arg1, irList2->code.arg1)) {
                                /* if(irList1->code.arg1.u.tmpId == irList2->code.arg1.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_ADD;
                                    p->code.arg1 = irList1->code.arg2;
                                    p->code.arg2 = irList2->code.arg2;
                                } else if(isOperandEqual(irList1->code.arg2, irList2->code.arg1)) {
                                /* } else if(irList1->code.arg2.u.tmpId == irList2->code.arg1.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_ADD;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg2;
                                }
                            }
                        } else if(irList1->code.kind == IR_SUB && irList2->code.kind == IR_SUB) {
                            if((irList1->code.arg1.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) 
                                    && (irList1->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))
                                    && (irList2->code.arg1.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p)) 
                                    && (irList2->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {
                            /* if(irList1->code.arg1.kind == OP_TEMP && irList1->code.arg2.kind == OP_TEMP */
                            /*         && irList2->code.arg1.kind == OP_TEMP && irList2->code.arg2.kind == OP_TEMP) { */
                                if(isOperandEqual(irList1->code.arg1, irList2->code.arg1)) {
                                /* if(irList1->code.arg1.u.tmpId == irList2->code.arg1.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList2->code.arg2;
                                    p->code.arg2 = irList1->code.arg2;
                                } else if(isOperandEqual(irList1->code.arg2, irList2->code.arg2)) {
                                /* } else if(irList1->code.arg2.u.tmpId == irList2->code.arg2.u.tmpId) { */
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg1;
                                }
                            }
                        } else if(irList1->code.kind == IR_ADD && irList2->code.kind == IR_ADD) {
                            if((irList1->code.arg1.kind == OP_TEMP || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p)) 
                                    && (irList1->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList1->code.arg2), irList1, p))
                                    && (irList2->code.arg1.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p)) 
                                    && (irList2->code.arg2.kind == OP_TEMP|| checkOrder(HT_find(hashTable, irList2->code.arg2), irList2, p))) {
                                if(isOperandEqual(irList1->code.arg1, irList2->code.arg1)) {
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg2;
                                    p->code.arg2 = irList2->code.arg2;
                                } else if(isOperandEqual(irList1->code.arg1, irList2->code.arg2)) {
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg2;
                                    p->code.arg2 = irList2->code.arg1;
                                } else if(isOperandEqual(irList1->code.arg2, irList2->code.arg1)) {
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg2;
                                } else if(isOperandEqual(irList1->code.arg2, irList2->code.arg2)) {
                                    *changed = true;
                                    p->code.kind = IR_SUB;
                                    p->code.arg1 = irList1->code.arg1;
                                    p->code.arg2 = irList2->code.arg1;
                                }
                            }
                            }            

                    }
                }
                if(isOperandEqual(p->code.arg1, p->code.arg2) && p->code.kind == IR_SUB) {
                    *changed = true;
                    p->code.kind = IR_ASSIGN;
                    p->code.arg1.kind = OP_CONST;
                    p->code.arg1.u.value = 0;
                } else if(isOperandEqual(p->code.arg1, p->code.arg2) && p->code.kind == IR_DIV) {
                    *changed = true;
                    p->code.kind = IR_ASSIGN;
                    p->code.arg1.kind = OP_CONST;
                    p->code.arg1.u.value = 1;
                } else if(p->code.result.kind == OP_TEMP){
                    /* irList1 = lookback(p->prev, p); */
                    /* if(irList1) { */
                    /*     *changed = true; */
                    /*     p->code.kind = IR_ASSIGN; */
                    /*     p->code.arg1 = irList1->code.result; */
                    /*     /1* p->code.arg1.kind = irList1->code.result.kind; *1/ */
                    /*     /1* p->code.arg1.u = p->code.result.u; *1/ */
                    /* } */
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_REF:
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_DEREF_L:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.result);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p))) {
                    *changed = true;
                    p->code.result = irList2->code.arg1;
                }
                break;
            case IR_DEREF_R:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                HT_insert(hashTable, p->code.result, p);
                break;
            case IR_ARG:
            case IR_WRITE:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                break;
            /* case IR_GOTO: */
            /*     break; */
            case IR_RELOP:
                irList1 = HT_find(hashTable, p->code.arg1);
                irList2 = HT_find(hashTable, p->code.arg2);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                if(irList2 && irList2->code.kind == IR_ASSIGN && (irList2->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList2->code.arg1), irList2, p))) {
                    *changed = true;
                    p->code.arg2 = irList2->code.arg1;
                }
                /* HT_clear(hashTable); */
                break;
            case IR_RET:
                irList1 = HT_find(hashTable, p->code.arg1);
                if(irList1 && irList1->code.kind == IR_ASSIGN && (irList1->code.arg1.kind != OP_VAR || checkOrder(HT_find(hashTable, irList1->code.arg1), irList1, p))) {
                    *changed = true;
                    p->code.arg1 = irList1->code.arg1;
                }
                /* HT_clear(hashTable); */  // TODO: check the correctness
                break;
            /* case IR_DEC: */
            /*     break; */
            case IR_CALL:
                HT_insert(hashTable, p->code.result, p);
                break;
            /* case IR_PARM: */
            /*     break; */
            /* case IR_READ: */
            /*     break; */
            /* case IR_WRITE: */
            /*     break; */
            default:
                break;
        }
        p = p->next;
    } while(p != codeList);
    HT_clear(hashTable);
    free(hashTable);
}

void evalConst(bool *changed) {
    IRList *p = codeList;
    do {
        if(p->code.kind == IR_ADD) {
            // constant pre calculate
            if(p->code.arg1.kind == OP_CONST && p->code.arg2.kind == OP_CONST) {
                p->code.kind = IR_ASSIGN;
                /* p->code.arg1.kind = OP_CONST; */
                p->code.arg1.u.value = p->code.arg1.u.value + p->code.arg2.u.value;
                *changed = true;
            } else if(p->code.arg1.kind == OP_CONST && p->code.arg1.u.value == 0) {
                p->code.kind = IR_ASSIGN;
                p->code.arg1 = p->code.arg2;
                *changed = true;
            } else if(p->code.arg2.kind == OP_CONST && p->code.arg2.u.value == 0) {
                p->code.kind = IR_ASSIGN;
                *changed = true;
                /* p->code.arg1 = p->code.arg1; */
            }
        } else if(p->code.kind == IR_SUB) {
            if(p->code.arg1.kind == OP_CONST && p->code.arg2.kind == OP_CONST) {
                p->code.kind = IR_ASSIGN;
                /* p->code.arg1.kind = OP_CONST; */
                p->code.arg1.u.value = p->code.arg1.u.value - p->code.arg2.u.value;
                *changed = true;
            } else if(p->code.arg2.kind == OP_CONST && p->code.arg2.u.value == 0) {
                p->code.kind = IR_ASSIGN;
                *changed = true;
                /* p->code.arg1 = p->code.arg1; */
            }
        } else if(p->code.kind == IR_MUL) {
            if(p->code.arg1.kind == OP_CONST && p->code.arg2.kind == OP_CONST) {
                p->code.kind = IR_ASSIGN;
                /* p->code.arg1.kind = OP_CONST; */
                p->code.arg1.u.value = p->code.arg1.u.value * p->code.arg2.u.value;
                *changed = true;
            } else if(p->code.arg1.kind == OP_CONST && p->code.arg1.u.value == 1) {
                p->code.kind = IR_ASSIGN;
                p->code.arg1 = p->code.arg2;
                *changed = true;
            } else if(p->code.arg2.kind == OP_CONST && p->code.arg2.u.value == 1)  {
                p->code.kind = IR_ASSIGN;
                *changed = true;
                /* p->code.arg1 = p->code.arg1; */
            } else if((p->code.arg1.kind == OP_CONST && p->code.arg1.u.value == 0)
                    || (p->code.arg2.kind == OP_CONST && p->code.arg2.u.value == 0)) {
                p->code.kind = IR_ASSIGN;
                p->code.arg1.kind = OP_CONST;
                p->code.arg1.u.value = 0;
                *changed = true;
            }
        } else if(p->code.kind == IR_DIV) {
            if(p->code.arg1.kind == OP_CONST && p->code.arg2.kind == OP_CONST) {
                p->code.kind = IR_ASSIGN;
                p->code.arg1.kind = OP_CONST;
                int a = p->code.arg1.u.value;
                int b = p->code.arg2.u.value;
                p->code.arg1.u.value = a / b;
                if(a % b) {
                    p->code.arg1.u.value -= (double)a / b < 0;
                }
                /* p->code.arg1.u.value -= p->code.arg1.u.value <= 0;   // floor */
                *changed = true;
            } else if(p->code.arg1.kind == OP_CONST && p->code.arg1.u.value == 0) {
                p->code.kind = IR_ASSIGN;
                p->code.arg1.u.value = 0;
                *changed = true;
            } else if(p->code.arg2.kind == OP_CONST && p->code.arg2.u.value == 1) {
                p->code.kind = IR_ASSIGN;
                *changed = true;
                /* p->code.arg1 = p->code.arg1; */
            }
        }
        p = p->next;
    } while(p != codeList);
}

void assignElimit(bool *changed) {
    DeadCode *deadList = NULL;
    DeadCode *deadCode = NULL;
    IRList *p = codeList;
    /* p = codeList; */
    do {
        switch(p->code.kind) {
            case IR_ASSIGN:
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
            case IR_REF:
            case IR_DEREF_R:
            /* case IR_CALL: */
                deadCode = (DeadCode*)malloc(sizeof(DeadCode));
                deadCode->irList = p;
                deadCode->next = deadList;
                deadList = deadCode;
                break;
            default:
                break;
        }
        p = p->next;
    } while(p != codeList);

    // check if is used
    /* p = codeList; */
    do {
        switch(p->code.kind) {
            // uniary operator
            case IR_ASSIGN:
            case IR_REF:
            case IR_DEREF_R:
            case IR_RET:
            case IR_ARG:
            case IR_WRITE:
                deadList = updateDeadList(deadList, p->code.arg1);
                break;
            case IR_DEREF_L:
                deadList = updateDeadList(deadList, p->code.result);
                deadList = updateDeadList(deadList, p->code.arg1);
                break;
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
            case IR_RELOP:
                deadList = updateDeadList(deadList, p->code.arg1);
                deadList = updateDeadList(deadList, p->code.arg2);
                break;
            default:
                break;
        }
        p = p->next;
    } while(p != codeList);

    if(deadList) {
        *changed = true;
    }
    // delete all dead code
    while(deadList) {
        DeadCode *p = deadList;
        deadList = deadList->next;
        removeCode(p->irList);
        free(p);
    }

}

IRList *lookback(IRList *list, IRList *p) {
    assert(list && p);
    IRList *res = NULL;
    while(list != codeList) {
        /* switch(list->code.kind) { */
        /*     case IR_LABEL: */
        /*     case IR_FUNC: */
        /*         goto L1; */
        /*         break; */
        /*     case IR_ASSIGN: */
        /*     case IR_REF: */
        /*     case IR_DEREF_R: */
        /*     case IR_CALL: */
        /*         if(isOperandEqual(list->code.result, p->code.arg1) */ 
        /*                 || isOperandEqual(list->code.result, p->code.arg2)) { */
        /*             goto L1; */
        /*         } */
        /*         break; */
        /*     case IR_ADD: */
        /*     case IR_SUB: */
        /*     case IR_MUL: */
        /*     case IR_DIV: */
        /*         if(isOperandEqual(list->code.result, p->code.arg1) */ 
        /*                 || isOperandEqual(list->code.result, p->code.arg2)) { */
        /*             goto L1; */
        /*         } */
        /*         if(!isOperandEqual(list->code.result, p->code.result)) { */
        /*             if(list->code.kind == p->code.kind */
        /*                     && isOperandEqual(list->code.arg1, p->code.arg1) */ 
        /*                     && isOperandEqual(list->code.arg2, p->code.arg2)) { */
        /*                 res = list; */
        /*                 /1* break; *1/ */
        /*                 goto L1; */
        /*             } */
        /*         } */
        /*         break; */
        /*     default: */
        /*         break; */
        /* } */
        if(list->code.kind == IR_LABEL 
                || list->code.kind == IR_FUNC) {
            break;
        }
        if(isOperandEqual(list->code.result, p->code.arg1) 
                || isOperandEqual(list->code.result, p->code.arg2)) {
            break;
        }
        if(!isOperandEqual(list->code.result, p->code.result)) {
            if(list->code.kind == p->code.kind
                    && isOperandEqual(list->code.arg1, p->code.arg1) 
                    && isOperandEqual(list->code.arg2, p->code.arg2)) {
                res = list;
                break;
            }
        }
        list = list->prev;
    }
/* L1: return res; */
    return res;
}

void labelElimit(bool *changed) {
    IRList *p = codeList;
    int state = 0;  // 0 means last code is not label, 1 means last code is a label
    do {
        if(p->code.kind == IR_LABEL) {
            if(state == 0) {
                state = 1;
            } else {    // state = 1, continued label
                int labelId = p->code.arg1.u.labelId;
                IRList *pp = codeList;
                do {
                    if(pp->code.kind == IR_GOTO && pp->code.arg1.u.labelId == labelId) {
                        pp->code.arg1.u.labelId = p->prev->code.arg1.u.labelId;
                    } else if(pp->code.kind == IR_RELOP && pp->code.result.u.labelId == labelId) {
                        pp->code.result.u.labelId = p->prev->code.arg1.u.labelId;
                    }
                    pp = pp->next;
                } while(pp != codeList);
                p = p->prev;
                *changed = true;
                removeCode(p->next);
            }
        } else {
            state = 0;
        }
        p = p->next;
    } while(p != codeList);
}
bool isModifyInstr(IRList *irList, Operand op) {
    switch(irList->code.kind) {
        case IR_ASSIGN:
        case IR_DEREF_R:
        case IR_REF:
            return !isOperandEqual(irList->code.arg1, op);
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            return !isOperandEqual(irList->code.arg1, op)
                && !isOperandEqual(irList->code.arg2, op);
        default:
            return false;
    }
}

void last_optimize() {
    bool changed;
    substitute();
    /* assignSubs(&changed); */
    assignElimit(&changed);
}

void substitute() {
    IRList *p = codeList;
    IRList *irList = NULL;
    do {
        switch(p->code.kind) {
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
                irList = lookback(p->prev, p);
                if(irList) {
                    p->code.kind = IR_ASSIGN;
                    p->code.arg1 = irList->code.result;
                }
                break;
            default:
                break;
        }
        p = p->next;
    } while(p != codeList);
}

// check if p1 is before p2
bool checkOrder(IRList *p1, IRList *p2, IRList *end) {
    static int i = 0;
    i++;
    /* return !p1; */
    if(!p1) return true;
    while(true) {
        /* printf("%d\n", i); */
        if(p1 == end) {
            return false;
        } else if(p2 == end) {
            return true;
        }
        p1 = p1->next;
        p2 = p2->next;
    }
}
