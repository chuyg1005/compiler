#include "semantic.h"
#include <assert.h>

static RB_Tree *symbolTable = NULL;
static Symbol *func = NULL; // current function: for check return type
/* static Type *retType = NULL; */
static Type *intType = NULL;
static Type *floatType = NULL;
static int alloc_id = 1;    // for anonymous structure, simulate java anonymous class
static void semantic_error(int errType, int lineno, const char *desc, const char *text);    // output semantic error

// build symbol table
static void initSymbolTable();
static int symbolCmp(Symbol*, Symbol*);

// high level semantic parse
static void parse(Node *node);
static void parseExtDef(Node *extDef);
static void parseDef(Node *def);
static void checkStmt(Node *stmt);

// low level semantic parse
static void parseFunDec(Type *type, Node *funDec);
static void parseExtDecList(Type *type, Node *extDecList);
static void parseDecList(Type *type, Node *decList);

// utilities
static Type* parseSpecifier(Node *specifier);
static Type* parseStructSpecifier(Node *structSpecifier);
/* static FieldList* buildFields(Node *defList); */
static FieldList* buildFields(FieldList *fieldList, Node *defList);
static ArgList* buildArgs(Node *varList);
static Symbol* getVarSymbol(Type *type, Node *varDec);  // type is inherited attribute
static Symbol* getFunSymbol(Type *retType, Node *funDec);   // return type is inherited attribute
/* static Type* getExpType(Node *exp); // parse expression */
static Type* getComplexExpType(Node *exp);

static bool structEqual(FieldList*,FieldList*);
static bool typeEqual(Type*, Type*);
static FieldList* getField(FieldList *structure, const char *name);

static bool addField(FieldList *structure, FieldList *field);
static bool checkArgs(ArgList *argsList, Node *args);
static bool isLeftVal(Node *exp);
static void printSymbolTable();

// for memory dealloc
static void freeSymbol(Symbol *sym);
static void freeVar(Symbol *var);
static void freeFun(Symbol *fun);
static void freeStruct(Symbol *structure);

// for lab3(add read and write functions)
static void addBuiltInFuns();

void semantic_parse(Node *root) {
    initSymbolTable();
#ifdef __LAB3__
    addBuiltInFuns();
#endif
    parse(root);
    /* RB_check(symbolTable); */
    /* printSymbolTable(); */
#ifndef __LAB3__
    clearSymbolTable();
#endif
}

static void parse(Node *node) {
    if(node == NULL) return;
    /* if(node->type > NODE_Program && node->childno == 0) return; */
    switch(node->type) {
        case NODE_ExtDef:
            parseExtDef(node);
            return;
        case NODE_Stmt:
            checkStmt(node);
            return;
        case NODE_Def:
            parseDef(node);
            return;
        default:
            break;
    }
    for(Node *p = node->child; p; p = p->sib) {
        parse(p);
    }
}

static void parseExtDef(Node *extDef) {
    Type *type = parseSpecifier(extDef->child);
    if(!type) {
        // TODO: report error (undefined structure)
        /* fprintf(stderr, "test"); */
        semantic_error(17, extDef->lineno, "Undefined structure", extDef->child->child->child->sib->child->val.name);
        if(extDef->child->sib->type == NODE_FunDec) {
            parse(extDef->child->sib->sib);
        }
    } else if(extDef->child->sib->type == NODE_ExtDecList) {
        parseExtDecList(type,extDef->child->sib); 
    } else if(extDef->child->sib->type == NODE_FunDec) {
        parseFunDec(type,extDef->child->sib); 
        parse(extDef->child->sib->sib);
    }
}

static void parseDef(Node *def) {
    Type *type = parseSpecifier(def->child);
    if(!type) {
        semantic_error(17, def->lineno, "Undefined structure", def->child->child->child->sib->child->val.name);
        /* assert(0); */
        // TODO: report error (undefined structure)
    }
    else {
        parseDecList(type, def->child->sib);
    }
}

static void checkStmt(Node *stmt) {
    Type *type = NULL;
    switch(stmt->child->type) {
        case NODE_Exp:
            getExpType(stmt->child);
            break;
        case NODE_CompSt:
            parse(stmt->child);
            break;
        case NODE_RETURN:
            if(func && !typeEqual(getExpType(stmt->child->sib), func->u.func->retType)) {
                // TODO: return type mismatch
                /* fprintf(stderr, "Type mismatched for return.\n"); */
                semantic_error(8, stmt->lineno, "Type mismatched for return", NULL);
            }
            break;
        case NODE_IF:
        case NODE_WHILE:
            type = getExpType(stmt->child->sib->sib);
            if(type && (type->kind != BASIC || type->u.basic != TYPE_INT)) {
                // TODO: logical type error
                semantic_error(7, stmt->child->sib->sib->lineno, "Type mismatched for operands", NULL);
                /* fprintf(stderr, "Type mismatched for logical operation.\n"); */
            }
            checkStmt(stmt->child->sib->sib->sib->sib);
            if(stmt->childno == 7) {
                checkStmt(stmt->child->sib->sib->sib->sib->sib->sib);
            }
            break;
        default:
            assert(0);
    }
}

static void initSymbolTable() {
    assert(symbolTable == NULL);
    symbolTable = createRB_Tree(symbolCmp);
    // create primitive type node
    intType = (Type*)malloc(sizeof(Type));
    floatType = (Type*)malloc(sizeof(Type));
    intType->kind = BASIC;
    floatType->kind = BASIC;
    intType->u.basic = TYPE_INT;
    floatType->u.basic = TYPE_FLOAT;
    /* return createRB_Tree(symbolCmp); */
}

static int symbolCmp(Symbol *symbol1, Symbol *symbol2) {
    int r = strcmp(symbol1->name, symbol2->name);
    if(r == 0) {
        r = symbol1->kind - symbol2->kind;
    }
    /* printf("compare %s : %s, result = %d\n", symbol1->name, symbol2->name, r); */
    return r;
}

void clearSymbolTable() {
    /* printf("symbol table size = %d\n", symbolTable->size); */
    clearRB_Tree(symbolTable, freeSymbol);
    free(symbolTable);
    symbolTable = NULL;
    free(intType);
    free(floatType);
    intType = NULL;
    floatType = NULL;
}

Symbol* lookupSymbol(const char *name, SymbolKind kind) {
    /* printf("lookup symbol: %s, kind: %d\n", name, kind); */
    Symbol symbol = { .kind = kind };
    strcpy(symbol.name, name);
    return RB_find(symbolTable, &symbol);
}

static bool insertSymbol(Symbol *symbol) {
    /* printf("insert symbol: %s, kind: %d\n", symbol->name, symbol->kind); */
    return RB_insert(symbolTable, symbol);
}

static Type* parseSpecifier(Node *specifier) {
    assert(specifier->type == NODE_Specifier);
    // basic type
    if(specifier->child->type == NODE_TYPE) {
        if(strcmp(specifier->child->val.name, "int") == 0) {
            return intType;
        } else if(strcmp(specifier->child->val.name, "float") == 0) {
            return floatType;
        } else {
            assert(0);
        }
    }
    assert(specifier->child->type == NODE_StructSpecifier);
    return parseStructSpecifier(specifier->child);
}

static Type* parseStructSpecifier(Node *structSpecifier) {
    assert(structSpecifier->type == NODE_StructSpecifier);
    // struct tag
    if(structSpecifier->childno == 2) {
        Node *tag = structSpecifier->child->sib;
        Symbol *symbol = lookupSymbol(tag->child->val.name, SYM_STRUCT);
        return symbol == NULL ? NULL : symbol->u.type;
    }
    // struct opttag lc deflist rc
    assert(structSpecifier->childno == 5);
    Type *type = (Type*)malloc(sizeof(Type));
    type->kind = STRUCTURE;
    Node *optTag = structSpecifier->child->sib;
    Node *defList = optTag->sib->sib;
    assert(optTag->type == NODE_OptTag);
    assert(defList->type == NODE_DefList);
    /* type->u.structure = buildFields(defList); */
    type->u.structure = NULL;
    type->u.structure = buildFields(type->u.structure, defList);
    // non anonymous structure
    Symbol *symbol = (Symbol*)malloc(sizeof(Symbol));
    symbol->kind = SYM_STRUCT;
    if(optTag->child != NULL) {
        strcpy(symbol->name, optTag->child->val.name);
    } else {
        sprintf(symbol->name, "%d", alloc_id++);
    }
    symbol->u.type = type;
    // insert failed
    if(!insertSymbol(symbol)) {
        semantic_error(16, optTag->lineno, "Duplicated name", symbol->name);
        free(symbol);
    } else if(lookupSymbol(symbol->name, SYM_VAR)) {    // check id
        semantic_error(16, optTag->lineno, "Struct id duplicated with normal id", symbol->name);
    }
    return type;
}

static FieldList* buildFields(FieldList *fieldList, Node *defList) {
    assert(defList->type == NODE_DefList);
    // empty production
    if(!defList->child) {
        return fieldList;
    }
    Node *def = defList->child;
    Node *specifier = def->child;
    Node *decList = specifier->sib;
    Type *type = parseSpecifier(specifier);
    if(type) {
        while(true) {
           Node *dec = decList->child;

           Symbol *symbol = getVarSymbol(type, dec->child);
           FieldList *field = (FieldList*)malloc(sizeof(FieldList));
           field->next = NULL;
           strcpy(field->name, symbol->name);
           field->type = symbol->u.type;
           // tail insert
           if(!fieldList) {
                fieldList = field;
           } else if(!addField(fieldList, field)){
                // TODO: field duplicated with other variables
               semantic_error(15, dec->child->lineno, "Redefined field", symbol->name);
           }
           if(dec->childno == 3) {
               semantic_error(15, dec->child->lineno, "Illegal initialization of structure field", NULL);
           }
           free(symbol);

           /* if(dec->childno == 3) { */
           /*     semantic_error(15, dec->child->lineno, "Illegal initialization of structure field", NULL); */
           /*     /1* assert(0); *1/ */
           /*     // TODO: report error */
           /*     // struct field initialize is not allowed */
           /* } else { // dec->childno == 1 */
           /*     assert(dec->childno == 1); */
           /*     Symbol *symbol = getVarSymbol(type, dec->child); */
           /*     FieldList *field = (FieldList*)malloc(sizeof(FieldList)); */
           /*     field->next = NULL; */
           /*     strcpy(field->name, symbol->name); */
           /*     field->type = symbol->u.type; */
           /*     // tail insert */
           /*     if(!fieldList) { */
           /*          fieldList = field; */
           /*     } else if(!addField(fieldList, field)){ */
           /*          // TODO: field duplicated with other variables */
           /*         semantic_error(15, dec->child->lineno, "Redefined field", symbol->name); */
           /*     } */
           /*     free(symbol); */
           /* } */
           if(!dec->sib) break;     // single dec
           decList = dec->sib->sib; // dec comma declist
        }
    } else {
        semantic_error(17, specifier->lineno, "Undefined structure", specifier->child->child->sib->child->val.name);
    }
    return buildFields(fieldList, def->sib);
}

bool addField(FieldList *structure, FieldList *field) {
    assert(structure);
    assert(!field->next);
    FieldList *iter = structure;
    while(true) {
        if(strcmp(iter->name, field->name) == 0) {
            return false;
        }
        if(!iter->next) {
            break;
        }
        iter = iter->next;
    }
    /* field->next = NULL; */
    iter->next = field;
    return true;
}

static ArgList* buildArgs(Node *varList) {
    assert(varList->type == NODE_VarList);
    ArgList *argList = NULL;
    ArgList *tail = NULL;
    while(true) {
        Node *paramDec = varList->child;
        Node *specifier = paramDec->child;
        Node *varDec = specifier->sib;
        Type *type = parseSpecifier(specifier);
        if(type) {
            Symbol *symbol = getVarSymbol(type, varDec);
            if(!insertSymbol(symbol)) {
                // TODO: report error
                semantic_error(3, varDec->lineno, "Redefined variable", symbol->name);
                free(symbol);
                /* assert(0); */
                // function parameter duplicated with other global vars
            } else if(lookupSymbol(symbol->name, SYM_STRUCT)) {
                semantic_error(3, varDec->lineno, "Variable duplicated with struct id", symbol->name);
            }
            ArgList *arg = (ArgList*)malloc(sizeof(ArgList));
            arg->next = NULL;
            arg->type = symbol->u.type;
            if(argList == NULL) {
                argList = arg;
                tail = arg;
            } else {
                tail->next = arg;
                tail = arg;
            }
        } else {
            semantic_error(17, specifier->lineno, "Undefined structure", specifier->child->child->sib->child->val.name);
        }
        if(!paramDec->sib) break;
        varList = paramDec->sib->sib;
    }
    return argList;
}

static Symbol* getVarSymbol(Type *type, Node *varDec) {
    /* printf("test\n"); */
    Symbol *symbol = (Symbol*)malloc(sizeof(Symbol));
    symbol->kind = SYM_VAR;
    // primitive type
    if(varDec->childno == 1) {
        strcpy(symbol->name, varDec->child->val.name); 
        symbol->u.type = type;
    } else {    // array
        Type *prev = type;
        while(varDec->childno == 4) {
            Type *cur = (Type*)malloc(sizeof(Type));
            cur->kind = ARRAY;
            cur->u.array.elem = prev;
            cur->u.array.size = varDec->child->sib->sib->val.intVal;
            prev = cur;
            varDec = varDec->child;
        }
        strcpy(symbol->name, varDec->child->val.name);
        symbol->u.type = prev;
    }
    return symbol;
}

static Symbol* getFunSymbol(Type *retType, Node *funDec) {
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    symbol->kind = SYM_FUNC;
    strcpy(symbol->name, funDec->child->val.name);
    symbol->u.func = (Func*)malloc(sizeof(Func));
    symbol->u.func->retType = retType;
    symbol->u.func->argList = NULL;
    if(funDec->childno == 4) {
        symbol->u.func->argList = buildArgs(funDec->child->sib->sib);
    }
    return symbol;
}

static FieldList* getField(FieldList *structure, const char *name) {
    FieldList *iter = structure;
    while(iter) {
        if(strcmp(iter->name, name) == 0) break;
        iter = iter->next;
    }
    return iter;
}

Type* getExpType(Node *exp) {
    Node *first = exp->child;
    Type *subType = NULL;
    Symbol *symbol = NULL;
    // single child
    if(exp->childno == 1) {
        switch(first->type) {
            case NODE_ID:
                symbol = lookupSymbol(first->val.name, SYM_VAR);
                if(!symbol) {
                    // TODO: undefined id 
                    /* assert(0); */
                    semantic_error(1, first->lineno, "Undefined variable", first->val.name);
                    return NULL;
                }
                return symbol->u.type;
            case NODE_INT:
                return intType;
            case NODE_FLOAT:
                return floatType;
            default:
                assert(0);
        }
    }
    switch(first->type) {
        case NODE_LP:
            return getExpType(first->sib);
            break;
        case NODE_MINUS:
            /* assert(exp->childno == 2); */
            subType = getExpType(first->sib);
            if(subType && subType->kind != BASIC) {
                semantic_error(7, exp->lineno, "Type mismatched for operands", NULL);
                /* assert(0); */
                return NULL;
            }
            return subType;
            break;
        case NODE_NOT:
            subType = getExpType(first->sib);
            if(subType && (subType->kind != BASIC || subType->u.basic != TYPE_INT)) {
                // TODO: type miss match for logical operator
                semantic_error(7, exp->lineno, "Type mismatched for operands", NULL);
                return NULL;
            }
            return subType;
            break;
        case NODE_ID:   // function call
            /* printf("%s\n", first->val.name); */
            symbol = lookupSymbol(first->val.name, SYM_FUNC);
            if(!symbol) {
                // TODO: undefined function name
                /* assert(0); */
                if(lookupSymbol(first->val.name, SYM_VAR)) {
                    semantic_error(11, first->lineno, "It is not a function", first->val.name);
                    return NULL;
                }
                semantic_error(2, first->lineno, "Undefined function", first->val.name);
                return NULL;
            }
            if(exp->childno == 3) {
                if(!checkArgs(symbol->u.func->argList, NULL)) {
                    // TODO: arguments mismatch
                    /* assert(0); */
                    semantic_error(9, exp->lineno, "Function arguments not applicable", NULL);
                    return NULL;
                }
            } else if(exp->childno == 4) {
                if(!checkArgs(symbol->u.func->argList, first->sib->sib)) {
                    // TODO: arguments mismatch
                    /* assert(0); */
                    semantic_error(9, exp->lineno, "Function arguments not applicable", NULL);
                    return NULL;
                }
            } else {
                assert(0);
            }
            return symbol->u.func->retType;
            break;
        case NODE_Exp:
            return getComplexExpType(exp);
            break;
        default:
            assert(0);
    }
    return NULL;
}

static Type* getComplexExpType(Node *exp) {
    assert(exp->child->type == NODE_Exp);
    FieldList *field = NULL;
    Type *type = NULL;
    Type *first = NULL;
    Type *second = NULL;
    switch(exp->child->sib->type) {
        case NODE_ASSIGNOP:
            first = getExpType(exp->child);
            second = getExpType(exp->child->sib->sib);
            if(!isLeftVal(exp->child)) {
                /* assert(0); */
                semantic_error(6, exp->lineno, "The left-hand side of an assignment must be a variable", NULL);
                return NULL;
            } else if(!typeEqual(first, second)) {
                semantic_error(5, exp->lineno, "Type mismatched for assignment", NULL);
                /* assert(0); */
                return NULL;
            }
            return first;
            break;
        case NODE_AND:  // int
        case NODE_OR:
            first = getExpType(exp->child);
            second = getExpType(exp->child->sib->sib);
            if(!first || !second) {
                return NULL;
            }
            if(!typeEqual(first, second) || first->kind != BASIC || first->u.basic != TYPE_INT) {
            /* if(first->kind != second->kind || first->kind != BASIC || first->u.basic != second->u.basic || first->u.basic != TYPE_INT) { */
                // TODO: logical operand error: not int
                semantic_error(7, exp->lineno, "Type mismatched for operands", NULL);
                /* assert(0); */
                return NULL;
            }
            return first;
            break;
        case NODE_RELOP:    // primitive types, return int
            first = getExpType(exp->child);
            second = getExpType(exp->child->sib->sib);
            if(!first || !second) {
                return NULL;
            }
            if(!typeEqual(first, second) || first->kind != BASIC) {
                semantic_error(7, exp->lineno, "Type mismatched for operands", NULL);
                /* assert(0); */
                return NULL;
            }
            return intType;
            break;
        case NODE_PLUS: // primitive types
        case NODE_MINUS:
        case NODE_STAR:
        case NODE_DIV:
            first = getExpType(exp->child);
            second = getExpType(exp->child->sib->sib);
            if(!first || !second) {
                return NULL;
            }
            if(!typeEqual(first, second) || first->kind != BASIC) {
                semantic_error(7, exp->lineno, "Type mismatched for operands", NULL);
                /* assert(0); */
                return NULL;
            }
            return first;
            break;
        case NODE_LB:   // array use
            first = getExpType(exp->child);
            second = getExpType(exp->child->sib->sib);
            if(!first || !second) {
                return NULL;
            }
            if(first->kind != ARRAY) {
                // TODO: not an array
                semantic_error(10, exp->child->lineno, "It is not an array", NULL);
                /* fprintf(stderr, "%d, %d\n", second->kind, second->u.basic); */
                if(second->kind != BASIC || second->u.basic != TYPE_INT) {
                    semantic_error(12, exp->child->sib->sib->lineno, "Array index is not an integer", NULL);
                }
                return NULL;
            }
            if(second->kind != BASIC || second->u.basic != TYPE_INT) {
                semantic_error(12, exp->child->sib->sib->lineno, "Array index is not an integer", NULL);
                // TODO: not a integer
                /* assert(0); */
                return NULL;
            }
            return first->u.array.elem;
            break;
        case NODE_DOT:  // structure use
            first = getExpType(exp->child);
            if(!first) {
                return NULL;
            }
            if(first->kind != STRUCTURE) {
                // TODO: not a struture
                /* assert(0); */
                semantic_error(13, exp->lineno, "Illegal use of \".\"", NULL);
                return NULL;
            }
            field = getField(first->u.structure, exp->child->sib->sib->val.name);
            if(!field) {
                // TODO: field not found
                /* assert(0); */
                semantic_error(14, exp->lineno, "Non-existent field", exp->child->sib->sib->val.name);
                return NULL;
            }
            return field->type;
        default:
            assert(0);
    }
    return type;
}

static void parseExtDecList(Type *type, Node *extDecList) {
    Symbol *symbol = getVarSymbol(type, extDecList->child);
    if(!insertSymbol(symbol)) {
        semantic_error(3, extDecList->lineno, "Redefined variable", symbol->name);
        free(symbol);
        /* free(symbol); */
        // TODO: report error
        // global variable redefined
    } else if(lookupSymbol(symbol->name, SYM_STRUCT)) {
        semantic_error(3, extDecList->lineno, "Variable duplicated with struct id", symbol->name);
    }
    if(extDecList->childno == 3) parseExtDecList(type, extDecList->child->sib->sib);
}

static void parseDecList(Type *type, Node *decList) {
    Node *dec = decList->child;
    Symbol *symbol = getVarSymbol(type, dec->child);
    /* printf("%s\n", symbol->name); */
    if(dec->childno == 3 && !typeEqual(symbol->u.type, getExpType(dec->child->sib->sib))) {
        semantic_error(5, dec->lineno, "Type mismatched for assignment", symbol->name);
        // TODO: report error
        // type dismatch
    } else if(!insertSymbol(symbol)) {
        semantic_error(3, decList->lineno, "Redefined variable", symbol->name);
        free(symbol);
        /* free(symbol); */
        /* assert(0); */
        // TODO: report error
        // local variable redefined
    } else if(lookupSymbol(symbol->name, SYM_STRUCT)) {
        semantic_error(3, decList->lineno, "Variable duplicated with struct id", symbol->name);
    }
    if(decList->childno == 3) parseDecList(type, dec->sib->sib);
}

static void parseFunDec(Type *type, Node *funDec) {
    Symbol *symbol = getFunSymbol(type, funDec);
    /* printf("%s\n", symbol->name); */
    if(!insertSymbol(symbol)) {
        semantic_error(4, funDec->lineno, "Redefined function", symbol->name);
        /* free(symbol); */
        /* assert(0); */
        // TODO: report error
        // function name redefined
        /* func = NULL; */
    }
    func = symbol;
}
static bool structEqual(FieldList *st1, FieldList *st2) {
    if(st1 == NULL && st2 == NULL) return true;
    if(st1 == NULL || st2 == NULL) return false;
    if(!typeEqual(st1->type, st2->type)) return false;
    return structEqual(st1->next, st2->next);
}

static bool typeEqual(Type *t1, Type *t2) {
    /* if(t1 == NULL && t2 == NULL) return true; */
    /* if(t1 == NULL || t2 == NULL) return false; */
    if(t1 == NULL || t2 == NULL) return true;   // avoid chain errors
    if(t1->kind != t2->kind) return false;
    if(t1->kind == BASIC) return t1->u.basic == t2->u.basic;
    if(t1->kind == STRUCTURE) return structEqual(t1->u.structure, t2->u.structure);
    if(t1->kind == ARRAY) return typeEqual(t1->u.array.elem, t2->u.array.elem);
    assert(0);
    return true;
}

static bool checkArgs(ArgList *argList, Node *args) {
    if(argList == NULL && args == NULL) return true;
    if(argList == NULL || args == NULL) return false;
    if(typeEqual(argList->type, getExpType(args->child))) {
        if(args->childno == 1) {
            return argList->next == NULL;
        }
        assert(args->childno == 3);
        return checkArgs(argList->next, args->child->sib->sib);
    }
    return false;
}

static bool isLeftVal(Node *exp) {
    switch(exp->child->type) {
        case NODE_Exp:
            return exp->child->sib->type == NODE_LB
                    || exp->child->sib->type == NODE_DOT;
        case NODE_ID:
            return exp->childno == 1;
        default:
            return false;
    }
}

static void printSymbolTable() {
    assert(symbolTable != NULL);
    RB_Iterator *iter = iterator(symbolTable);
    while(hasNext(iter)) {
        Symbol *symbol = nextNode(iter);
        printf("kind = %d, name = %s\n", symbol->kind, symbol->name);
    }
}

static void semantic_error(int errType, int lineno, const char* desc, const char* text) {
    semerr++;
    if(text) {
        fprintf(stderr, "\033[31mError type %d at line %d: %s \"%s\".\n\033[0m", 
               errType, lineno, desc, text);
    } else {
        fprintf(stderr, "\033[31mError type %d at line %d: %s.\n\033[0m", 
               errType, lineno, desc);
    }
}

static void addBuiltInFuns() {
    // generate read function : return type is int, argument list is null
    Symbol *read = (Symbol*)malloc(sizeof(Symbol));
    read->kind = SYM_FUNC;
    strcpy(read->name, "read");
    read->u.func = (Func*)malloc(sizeof(Func));
    read->u.func->retType = intType;
    read->u.func->argList = NULL;
    
    // generate write function : return type is int, argument list is single int
    Symbol *write = (Symbol*)malloc(sizeof(Symbol));
    write->kind = SYM_FUNC;
    strcpy(write->name, "write");
    write->u.func = (Func*)malloc(sizeof(Func));
    write->u.func->retType = intType;
    ArgList *argList = (ArgList*)malloc(sizeof(ArgList));
    argList->next = NULL;
    argList->type = intType;
    write->u.func->argList = argList;

    // insert read and write into symbol table
    assert(insertSymbol(read));
    assert(insertSymbol(write));
}


static void freeSymbol(Symbol *sym) {
    if(!sym) return;
    switch(sym->kind) {
        case SYM_VAR:
            freeVar(sym);
            break;
        case SYM_FUNC:
            freeFun(sym);
            break;
        case SYM_STRUCT:
            freeStruct(sym);
            break;
    }
    free(sym);
}

// free arraylist(linked list) if type is array
static void freeVar(Symbol *var) {
    /* printf("free var\n"); */
    assert(var);
    assert(var->kind == SYM_VAR);
    Type *type = var->u.type;
    while(type->kind == ARRAY) {
        Type *p = type;
        type = type->u.array.elem;
        free(p);
    }
    /* free(type); */
    return;
}

// free arglist(linked list) and func struct
static void freeFun(Symbol *fun) {
    /* printf("free func\n"); */
    assert(fun);
    assert(fun->kind == SYM_FUNC);
    Func *func = fun->u.func;
    ArgList *arg = func->argList;
    while(arg) {
        ArgList *p = arg;
        arg = arg->next;
        free(p);
    }
    free(func);
}

// free struture list(linked list)
static void freeStruct(Symbol *structure) {
    /* printf("free struct\n"); */
    assert(structure);
    assert(structure->kind == SYM_STRUCT);
    Type *type = structure->u.type;
    assert(type->kind == STRUCTURE);
    FieldList *field = type->u.structure;
    while(field) {
        FieldList *p = field;
        field = field->next;
        while(p->type->kind == ARRAY) {
            type = p->type;
            p->type = type->u.array.elem;
            free(type);
        }
        free(p);
    }
    free(structure->u.type);
}
