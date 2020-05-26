#include "oc.h"
#include <stdarg.h>
#include <assert.h>
#define REG_NUM 10

static Reg regs[REG_NUM];
static FILE* stream = NULL;
static LVList* lvList = NULL;   // local variable list
static IRList* codeList = NULL;
static int param_off = 0;
static int lv_off = 0;

void init_regs();

void gen_data_seg();
void gen_globl_seg();
void gen_text_seg();

void gen_read_func();
void gen_write_func();

Reg* get_reg(Operand op);
Reg* alloc_reg(Operand op);
void spill_reg(Reg *reg);
void spill_all_reg();
void free_reg(Reg *reg);
void free_all_reg();

LocalVar* get_local_var(Operand op);
LocalVar* add_local_var(Operand op, int size);
void add_param_var(Operand op);
void clear_lvList();

void enter_func();
void gen_prologue();
void gen_epilogue();

void generate_oc(IRList *irList, const char *filename) {
    stream = fopen(filename, "w");
    if(!stream) {
        perror("fopen");
        return;
    }
    codeList = irList;
    /* init_regs(); */
    gen_data_seg();
    gen_globl_seg();
    gen_text_seg();
    fclose(stream);
    clearIRList();
    clearSymbolTable();
}

void gen_data_seg() {
    fprintf(stream, ".data\n");
    fprintf(stream, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(stream, "_ret: .asciiz \"\\n\"\n");
}

void gen_globl_seg() {
    fprintf(stream, ".globl main\n");
}

void gen_read_func() {
    fprintf(stream, "\n");
    fprintf(stream, "read:\n");
    fprintf(stream, "  li $v0, 4\n");
    fprintf(stream, "  la $a0, _prompt\n");
    fprintf(stream, "  syscall\n");
    fprintf(stream, "  li $v0, 5\n");
    fprintf(stream, "  syscall\n");
    fprintf(stream, "  jr $ra\n");
}

void gen_write_func() {
    fprintf(stream, "\n");
    fprintf(stream, "write:\n");
    fprintf(stream, "  li $v0, 1\n");
    fprintf(stream, "  syscall\n");
    fprintf(stream, "  li $v0, 4\n");
    fprintf(stream, "  la $a0, _ret\n");
    fprintf(stream, "  syscall\n");
    fprintf(stream, "  move $v0, $0\n");
    fprintf(stream, "  jr $ra\n");
}

void gen_text_seg() {
    init_regs();
    fprintf(stream, ".text\n");
    gen_read_func();
    gen_write_func();
    IRList *p = codeList;
    LocalVar *p1 = NULL;
    Reg *x = NULL;
    Reg *y = NULL;
    Reg *z = NULL;
    do {
        switch(p->code.kind) {
            case IR_FUNC:
                /* spill_all_reg(); */
                /* clear_lvList(); */
                enter_func();
                fprintf(stream, "\n");
                fprintf(stream, "%s:\n", p->code.arg1.u.symbol->name);
                gen_prologue();
                break;
            case IR_LABEL:
                spill_all_reg();
                fprintf(stream, "label_%d:\n", p->code.arg1.u.labelId);
                break;
            case IR_ASSIGN: // x = y
                if(p->code.arg1.kind == OP_CONST) {
                    x = alloc_reg(p->code.result);
                    x->modified = true;
                    fprintf(stream, "  li %s, %d\n", x->name, p->code.arg1.u.value);
                } else {
                    y = get_reg(p->code.arg1);
                    x = alloc_reg(p->code.result);
                    x->modified = true;
                    fprintf(stream, "  move %s, %s\n", x->name, y->name);
                }
                break;
            case IR_ADD:    // z = x + y
                if(p->code.arg1.kind == OP_CONST) {
                    y = get_reg(p->code.arg2);
                    z = alloc_reg(p->code.result);
                    z->modified = true;
                    fprintf(stream, "  addi %s, %s, %d\n", z->name, y->name, p->code.arg1.u.value);
                } else {
                    x = get_reg(p->code.arg1);
                    x->locked = true;
                    y = get_reg(p->code.arg2);
                    x->locked =false;
                    z = alloc_reg(p->code.result);
                    z->modified = true;
                    fprintf(stream, "  add %s, %s, %s\n", z->name, x->name, y->name);
                }
                break;
            case IR_SUB:
                x = get_reg(p->code.arg1);
                x->locked = true;
                y = get_reg(p->code.arg2);
                x->locked =false;
                z = alloc_reg(p->code.result);
                z->modified = true;
                fprintf(stream, "  sub %s, %s, %s\n", z->name, x->name, y->name);
                break;
            case IR_MUL:
                x = get_reg(p->code.arg1);
                x->locked = true;
                y = get_reg(p->code.arg2);
                x->locked =false;
                z = alloc_reg(p->code.result);
                z->modified = true;
                fprintf(stream, "  mul %s, %s, %s\n", z->name, x->name, y->name);
                break;
            case IR_DIV:
                x = get_reg(p->code.arg1);
                x->locked = true;
                y = get_reg(p->code.arg2);
                x->locked =false;
                z = alloc_reg(p->code.result);
                z->modified = true;
                fprintf(stream, "  div %s, %s\n", x->name, y->name);
                fprintf(stream, "  mflo %s\n", z->name);
                break;
            case IR_REF:
                x = alloc_reg(p->code.result);
                x->modified = true;
                p1 = get_local_var(p->code.arg1);
                fprintf(stream, "  la %s, %d($fp)\n", x->name, p1->off);
                break;
            case IR_DEREF_L:    // *x = y
                y = get_reg(p->code.arg1);
                y->locked = true;
                x = get_reg(p->code.result);
                y->locked = false;
                fprintf(stream, "  sw %s, 0(%s)\n", y->name, x->name);
                break;
            case IR_DEREF_R:    // x = *y
                y = get_reg(p->code.arg1);
                /* y->locked = true; */
                x = alloc_reg(p->code.result);
                x->modified = true;
                /* y->locked = false; */
                fprintf(stream, "  lw %s, 0(%s)\n", x->name, y->name);
                break;
            case IR_GOTO:
                spill_all_reg();
                fprintf(stream, "  j label_%d\n", p->code.arg1.u.labelId);
                break;
            case IR_RELOP:
                x = get_reg(p->code.arg1);
                x->locked = true;
                y = get_reg(p->code.arg2);
                x->locked = false;
                spill_all_reg();
                switch(p->code.u.relop) {
                    case RELOP_EQ:
                        fprintf(stream, "  beq ");
                        break;
                    case RELOP_LE:
                        fprintf(stream, "  ble ");
                        break;
                    case RELOP_LT:
                        fprintf(stream, "  blt ");
                        break;
                    case RELOP_GE:
                        fprintf(stream, "  bge ");
                        break;
                    case RELOP_GT:
                        fprintf(stream, "  bgt ");
                        break;
                    case RELOP_NE:
                        fprintf(stream, "  bne ");
                        break;
                }
                fprintf(stream, "%s, %s, label_%d\n", x->name, y->name, p->code.result.u.labelId);
                break;
            case IR_RET:
                // spill_all_reg();    // no global variables
                x = get_reg(p->code.arg1);
                fprintf(stream, "  move $v0, %s\n", x->name);
                gen_epilogue();
                fprintf(stream, "  jr $ra\n");
                break;
            case IR_DEC:
                add_local_var(p->code.result, p->code.arg1.u.value);
                break;
            case IR_ARG:
                x = get_reg(p->code.arg1);
                fprintf(stream, "  addi $sp, $sp, -4\n");
                fprintf(stream, "  sw %s, 0($sp)\n", x->name);
                break;
            case IR_CALL:
                spill_all_reg();
                fprintf(stream, "  addi $sp, $sp, -4\n");
                fprintf(stream, "  sw $ra, 0($sp)\n");
                fprintf(stream, "  jal %s\n", p->code.arg1.u.symbol->name);
                fprintf(stream, "  lw $ra, 0($sp)\n");
                fprintf(stream, "  addi $sp, $sp, 4\n");
                x = alloc_reg(p->code.result);
                x->modified = true;
                fprintf(stream, "  move %s, $v0\n", x->name);
                break;
            case IR_PARM:
                add_param_var(p->code.arg1);
                break;
            case IR_READ:
                x = alloc_reg(p->code.arg1);
                x->modified = true;
                fprintf(stream, "  addi $sp, $sp, -4\n");
                fprintf(stream, "  sw $ra, 0($sp)\n");
                fprintf(stream, "  jal read\n");
                fprintf(stream, "  lw $ra, 0($sp)\n");
                fprintf(stream, "  addi $sp, $sp, 4\n");
                fprintf(stream, "  move %s, $v0\n", x->name);
                break;
            case IR_WRITE:
                x = get_reg(p->code.arg1);
                fprintf(stream, "  move $a0, %s\n", x->name);
                fprintf(stream, "  addi $sp, $sp, -4\n");
                fprintf(stream, "  sw $ra, 0($sp)\n");
                fprintf(stream, "  jal write\n");
                fprintf(stream, "  lw $ra, 0($sp)\n");
                fprintf(stream, "  addi $sp, $sp, 4\n");
                break;
        }
        p = p->next;
    } while(p != codeList);
}

void init_regs() {
    for(int i = 0; i < REG_NUM; i++) {
        regs[i].used = false;
        regs[i].modified = false;
        regs[i].locked = false;
        regs[i].var = NULL;
        sprintf(regs[i].name, "$t%d", i);
    }
}

Reg* get_reg(Operand op) {
    // variable is in the regs
    for(int i = 0; i < REG_NUM; i++) {
        if(regs[i].used && isOperandEqual(regs[i].var->op, op)) {
            return regs + i;
        }
    }
    // not in, need to load from the memory
    Reg *reg = alloc_reg(op);
    if(op.kind == OP_CONST) {
        reg->modified = true;
        fprintf(stream, "  li %s, %d\n", reg->name, op.u.value);
    } else if(op.kind == OP_TEMP || op.kind == OP_VAR) {
        fprintf(stream, "  lw %s, %d($fp)\n", reg->name, reg->var->off);
    } else {
        assert(0);
    }
    return reg;
}

Reg* alloc_reg(Operand op) {
    int i = 0;
    for(; i < REG_NUM; i++) {
        if(regs[i].used && isOperandEqual(regs[i].var->op, op)) {
            return regs + i;
        }
    }
    for(i = 0; i < REG_NUM; i++) {
        if(!regs[i].used) break;
    }
    if(i == REG_NUM) {
        for(i = 0; i < REG_NUM; i++) {
            if(!regs[i].locked) break;
        }
        if(i == REG_NUM) assert(0); // all regs is locked
        if(regs[i].modified) {
            spill_reg(regs + i);
            /* fprintf(stream, "  sw %s, %d($fp)\n", regs[i].name, regs[i].var->off); */
        }
    }
    regs[i].used = true;
    regs[i].modified = false;
    regs[i].locked = false;
    regs[i].var = get_local_var(op);
    return regs + i;
}

// get local variable from local variable list
LocalVar* get_local_var(Operand op) {
    LVList *p = lvList;
    while(p) {
        if(isOperandEqual(p->var.op, op)) break;
        p = p->next;
    }
    return p == NULL ? add_local_var(op, 4) : &p->var;
}

LocalVar* add_local_var(Operand op, int size) {
    LVList *p = (LVList*)malloc(sizeof(LVList));
    p->next = lvList;
    lvList = p;
    p->var.op = op;
    lv_off -= size; // allocate memory for op
    p->var.off = lv_off;
    fprintf(stream, "  addi $sp, $sp, -%d\n", size);
    return &p->var;
}

void add_param_var(Operand op) {
    LVList *p = (LVList*)malloc(sizeof(LVList));
    p->next = lvList;
    lvList = p;
    p->var.op = op;
    param_off += 4;
    p->var.off = param_off;
}

void spill_reg(Reg *reg) {
    if(reg->used && reg->modified) {
        fprintf(stream, "  sw %s, %d($fp)\n", reg->name, reg->var->off);
    }
    free_reg(reg);
}

void free_reg(Reg *reg) {
    reg->used = false;
    reg->modified = false;
    reg->locked = false;
    reg->var = NULL;
}

void free_all_reg() {
    for(int i = 0; i < REG_NUM; i++) {
        free_reg(regs+i);
    }
}

void spill_all_reg() {
    for(int i = 0; i < REG_NUM; i++) {
        spill_reg(regs + i);
    }
}

void clear_lvList() {
    while(lvList) {
        LVList *p = lvList;
        lvList = p->next;
        free(p);
    }
}

void enter_func() {
    free_all_reg();
    clear_lvList();
    lv_off = 0;
    param_off = 4;
}

void gen_prologue() {
    fprintf(stream, "  addi $sp, $sp, -4\n");
    fprintf(stream, "  sw $fp, 0($sp)\n");
    fprintf(stream, "  move $fp, $sp\n");
}

void gen_epilogue() {
    fprintf(stream, "  move $sp, $fp\n");
    fprintf(stream, "  lw $fp, 0($sp)\n");
    fprintf(stream, "  addi $sp, $sp, 4\n");
}
