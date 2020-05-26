%{
#include "Node.h"
#include "lex.yy.c"
void yyerror(const char*);
extern Node* root;  /* root of syntax tree */
extern int errnum;  /* syntax error num */ 
extern void synerror(const char*);  /* report error when miss syntax error */
extern YYLTYPE errloc;
%}

%union {
    Node* node; /* each grammer symbol is syntax tree node */
}

/* free user-allocated memory when syntax error */
%destructor { 
    /* some grammar symbol is assigned NULL
     * to prevent NullPointException.
     */
    if($$ != NULL)  {
        Log("\033[31mfree %s at line %d.\n\033[0m", $$->type, @$.first_line);
        freeTree($$);   /* prevent memory leak */
    }
} <node>
/* prevent destructor start symbol when the parser succeeds. */
%destructor { /* do nothing */ } Program

%locations /* enable yylloc */

%token <node> INT FLOAT ID  /* variable and literal */
%token <node> SEMI COMMA  /* delimiter */
%token <node> ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT  /* operator */
%token <node> LP RP LB RB LC RC  /* various parenthese */
%token <node> TYPE STRUCT RETURN IF ELSE WHILE  /* keyword */

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS 
%left LP RP LB RB DOT 

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
/* %nonassoc UMINUS */

/* %nonassoc LOWER_THAN_ELSE */
/* %nonassoc ELSE */
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args

%start Program  /* set start symbol explicitly */

%%
Program: ExtDefList {
    $$ = createNode("Program", @$.first_line);
    addChild($$, 1, $1);
    root = $$;
    Log("Program -> ExtDefList\n");
}   ;
ExtDefList: { 
    /* $$ = createNode("ExtDefList", @$.first_line); */
    $$ = NULL;
    Log("ExtDefList -> e\n");
}   | ExtDef ExtDefList {
    $$ = createNode("ExtDefList", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("ExtDefList -> ExtDef ExtDefList\n");
}   ;
ExtDef: Specifier ExtDecList SEMI {
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("ExtDef -> Specifier ExtDecList SEMI\n");
}   | Specifier SEMI {
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("ExtDef -> Specifier SEMI\n");
}   | Specifier FunDec CompSt {
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("ExtDef -> Specifier FunDec CompSt\n");
}   | Specifier error { /* struct declaration without simicolon */
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 2, createNode("Error", @$.first_line), $1);
    Log("ExtDef -> Specifier error\n");
    synerror("Missing \";\"");
    /* if(++errnum >= 10) YYABORT; */
}   | Specifier error SEMI { 
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 3, $3, createNode("Error", @$.first_line), $1);
    Log("ExtDef -> Specifier error\n");
    synerror("Global variables definition list error");
    /* if(++errnum >= 10) YYABORT; */
}   | error SEMI {
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 2, $2, createNode("Error", @$.first_line));
    Log("ExtDef -> error SEMI\n");
    synerror("Syntax error");
    /* if(++errnum >= 10) YYABORT; */
}   | Specifier ExtDecList error {
    $$ = createNode("ExtDef", @$.first_line);
    addChild($$, 3, createNode("Error", @$.first_line), $2, $1);
    Log("ExtDef -> error SEMI\n");
    synerror("Missing \";\"");
    /* if(++errnum >= 10) YYABORT; */
/* }   | Specifier error CompSt { */
/*     $$ = createNode("ExtDef", @$.first_line); */
/*     addChild($$, 3, $3, createNode("Error", @$.first_line), $1); */
/*     /1* Log("ExtDef -> error SEMI\n"); *1/ */
/*     synerror("Missing \";\""); */
/*     if(++errnum >= 10) YYABORT; */
};
ExtDecList: VarDec {
    $$ = createNode("ExtDecList", @$.first_line);
    addChild($$, 1, $1);
    Log("ExtDecList -> VarDec\n");
}   | VarDec COMMA ExtDecList {
    $$ = createNode("ExtDecList", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("ExtDecList -> VarDec COMMA ExtDecList\n");
}   ;

Specifier: TYPE {
    $$ = createNode("Specifier", @$.first_line);
    addChild($$, 1, $1);
    Log("Specifier -> TYPE\n");
}   | StructSpecifier {
    $$ = createNode("Specifier", @$.first_line);
    addChild($$, 1, $1);
    Log("Specifier -> StructSpecifier\n");
}   ;
StructSpecifier: STRUCT OptTag LC DefList RC {
    $$ = createNode("StructSpecifier", @$.first_line);
    addChild($$, 5, $5, $4, $3, $2, $1);
    Log("StructSpecifier -> STRUCT OptTag LC DefList RC\n");
}   | STRUCT Tag {
    $$ = createNode("StructSpecifier", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("StructSpecifier -> STRUCT Tag\n");
}   | STRUCT OptTag LC error RC {
    $$ = createNode("StructSpecifier", @$.first_line);
    addChild($$, 5, $5, createNode("Error", @$.first_line), $3, $2, $1);
    synerror("Struct definition error");
    Log("StructSpecifier -> STRUCT OptTag LC error RC\n");
    /* if(++errnum >= 10) YYABORT; */
/* }   | STRUCT OptTag LC DefList error { */
/*     $$ = createNode("StructSpecifier", @$.first_line); */
/*     addChild($$, 5, createNode("Error", @$.first_line), $4, $3, $2, $1); */
/*     synerror("Missing \";\""); */
/*     Log("StructSpecifier -> STRUCT OptTag LC DefList error\n"); */
/*     if(++errnum >= 10) YYABORT; */
};
OptTag: ID {
    $$ = createNode("OptTag", @$.first_line);
    addChild($$, 1, $1);
    Log("OptTag -> ID\n");
}   | {
    /* $$ = createNode("OptTag", @$.first_line); */
    $$ = NULL;
    Log("OptTag -> e\n");
}   ;
Tag: ID {
    $$ = createNode("Tag", @$.first_line);
    addChild($$, 1, $1);
    Log("Tag -> ID\n");
}   ;

VarDec: ID {
    $$ = createNode("VarDec", @$.first_line);
    addChild($$, 1, $1);
    Log("VarDec -> ID\n");
}   | VarDec LB INT RB {
    $$ = createNode("VarDec", @$.first_line);
    addChild($$, 4, $4, $3, $2, $1);
    Log("VarDec -> VarDec LB INT RB");
}   | VarDec LB error RB {  /* only int value is allowed */
    $$ = createNode("VarDec", @$.first_line);
    addChild($$, 4, $4, createNode("Error", @$.first_line), $2, $1);
    synerror("Array index error");
    Log("VarDec -> VarDec LB error RB");
    /* if(++errnum >= 10) YYABORT; */
}   | VarDec LB INT error {
    $$ = createNode("VarDec", @$.first_line);
    addChild($$, 4, createNode("Error", @$.first_line), $3, $2, $1);
    synerror("Missing \"]\"");
    Log("VarDec -> VarDec LB INT error");
    /* if(++errnum >= 10) YYABORT; */
};
FunDec: ID LP VarList RP {
    $$ = createNode("FunDec", @$.first_line);
    addChild($$, 4, $4, $3, $2, $1);
    Log("FunDec -> ID LP VarList RP\n");
}   | ID LP RP {
    $$ = createNode("FunDec", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("FunDec -> ID LP RP\n");
/* }   | ID LP error { */
/*     $$ = createNode("FunDec", @$.first_line); */
/*     addChild($$, 3, createNode("Error", @$.first_line), $2, $1); */
/*     synerror("Missing \")\""); */
/*     /1* Log("FunDec -> ID LP error RP\n"); *1/ */
/*     /1* yyerrok; *1/ */
/*     if(++errnum >= 10) YYABORT; */
}   | ID LP error RP { /* refactor */
    $$ = createNode("FunDec", @$.first_line);
    addChild($$, 4, $4, createNode("Error", @$.first_line), $2, $1);
    synerror("Function definition parameters error");
    Log("FunDec -> ID LP error RP\n");
    /* yyerrok; */
    /* if(++errnum >= 10) YYABORT; */
};
VarList: ParamDec COMMA VarList {
    $$ = createNode("VarList", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("VarList -> ParamDec COMMA VarList\n");
}   | ParamDec {
    $$ = createNode("VarList", @$.first_line);
    addChild($$, 1, $1);
    Log("VarList -> ParamDec\n");
}   ;
ParamDec: Specifier VarDec {
    $$ = createNode("ParamDec", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("ParamDec -> Specifier VarDec\n");
}   ;

CompSt: LC DefList StmtList RC {
    $$ = createNode("CompSt", @$.first_line);
    addChild($$, 4, $4, $3, $2, $1);
    Log("CompSt -> LC DefList StmtList RC\n");
}   | error RC {
    $$ = createNode("CompSt", @$.first_line);
    addChild($$, 2, $2, createNode("Error", @$.first_line));
    /* addChild($$, 3, $3, createNode("Error", @$.first_line), $1); */
    synerror("Syntax error");
    Log("CompSt -> error RC\n");
    /* if(++errnum <= 10) YYABORT; */
/* }   | LC DefList StmtList error { */
/*     $$ = createNode("CompSt", @$.first_line); */
/*     addChild($$, 4, createNode("Error", @$.first_line), $3, $2, $1); */
/*     synerror("Missing \";\""); */
/*     Log("CompSt -> LC error RC\n"); */
/*     if(++errnum <= 10) YYABORT; */
}   ;
StmtList: Stmt StmtList {
    $$ = createNode("StmtList", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("StmtList -> Stmt StmtList\n");
}   | {
    /* $$ = createNode("StmtList", @$.first_line); */
    $$ = NULL;
    Log("StmtList -> e\n");
}   ;
Stmt: Exp SEMI {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("Stmt -> Exp SEMI\n");
}   | CompSt {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 1, $1);
    Log("Stmt -> CompSt\n");
}   | RETURN Exp SEMI {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Stmt -> RETURN Exp SEMI\n");
}   | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{   /* TODO */
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 5, $5, $4, $3, $2, $1);
    Log("Stmt -> IF LP Exp RP Stmt\n");
}   | IF LP Exp RP Stmt ELSE Stmt {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 7, $7, $6, $5, $4, $3, $2, $1);
    Log("Stmt -> IF LP Exp RP Stmt ELSE Stmt\n");
}   | WHILE LP Exp RP Stmt {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 5, $5, $4, $3, $2, $1);
    Log("Stmt -> WHILE LP Exp RP Stmt\n");
}   | IF LP error RP Stmt %prec LOWER_THAN_ELSE {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 5, $5, $4, createNode("Error", @$.first_line), $2, $1);
    synerror("Logical expression error");
    Log("Stmt -> IF LP error RP Stmt");
    /* if(++errnum >= 10) YYABORT; */
/* }   | IF LP Exp error Stmt %prec LOWER_THAN_ELSE { */
/*     $$ = createNode("Stmt", @$.first_line); */
/*     addChild($$, 5, $5, createNode("Error", @$.first_line), $3, $2, $1); */
/*     synerror(@4.first_line, @4.first_column, "Missing \")\""); */
/*     Log("Stmt -> IF LP Exp error Stmt"); */
    /* if(++errnum >= 10) YYABORT; */
}   | IF LP error RP Stmt ELSE Stmt {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 7, $7, $6, $5, $4, createNode("Error", @$.first_line), $2, $1);
    synerror("Logical expression error");
    Log("Stmt -> IF LP error RP Stmt ELSE Stmt");
    /* if(++errnum >= 10) YYABORT; */
/* }   | IF LP Exp error Stmt ELSE Stmt { */
/*     $$ = createNode("Stmt", @$.first_line); */
/*     addChild($$, 7, $7, $6, $5, createNode("Error", @$.first_line), $3, $2, $1); */
/*     synerror(@4.first_line, @4.first_column, "Missing \")\""); */
/*     Log("Stmt -> IF LP Exp error Stmt ELSE Stmt"); */
    /* if(++errnum >= 10) YYABORT; */
}   | WHILE LP error RP Stmt {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 5, $5, $4, createNode("Error", @$.first_line), $2, $1);
    synerror("Logical expression error");
    Log("Stmt -> WHILE Lp error RP Stmt");
    /* if(++errnum >= 10) YYABORT; */
/* }   | WHILE LP Exp error Stmt { */
/*     $$ = createNode("Stmt", @$.first_line); */
/*     addChild($$, 5, $5, createNode("Error", @$.first_line), $3, $2, $1); */
/*     synerror("Missing \")\""); */
/*     Log("Stmt -> WHILE Lp Exp error Stmt"); */
    /* if(++errnum >= 10) YYABORT; */
}   | RETURN error SEMI {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 3, $3, createNode("Error", @$.first_line), $1);
    synerror("Return expression error");
    Log("Stmt -> RETURN error SEMI");
    /* if(++errnum >= 10) YYABORT; */
}   | RETURN Exp error {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 3, createNode("Error", @$.first_line), $2, $1);
    synerror("Missing \";\"");
    Log("Stmt -> RETURN Exp error");
    /* if(++errnum >= 10) YYABORT; */
}   | error SEMI {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 2, $2, createNode("Error", @$.first_line));
    synerror("Syntax error");
    Log("Stmt -> error SEMI");
    /* if(++errnum >= 10) YYABORT; */
}   | Exp error {
    $$ = createNode("Stmt", @$.first_line);
    addChild($$, 2, createNode("Error", @$.first_line), $1);
    synerror("Syntax error");
    Log("Stmt -> Exp error");
    /* if(++errnum >= 10) YYABORT; */
};

DefList: {
    $$ = NULL;
    /* $$ = createNode("DefList", @$.first_line); */
    Log("DefList -> e\n");
}   | Def DefList {
    $$ = createNode("DefList", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("DefList -> Def DefList\n");
}   ;
Def: Specifier DecList SEMI {
    $$ = createNode("Def", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Def -> Specifier DecList SEMI\n");
}   | Specifier error SEMI {    /* lost variables */
    $$ = createNode("Def", @$.first_line);
    addChild($$, 3, $3, createNode("Error", @$.first_line), $1);
    synerror("Local variables definition list error");
    Log("Def -> Specifier DecList SEMI\n");
    /* if(++errnum >= 10) YYABORT; */
};
DecList: Dec {
    $$ = createNode("DecList", @$.first_line);
    addChild($$, 1, $1);
    Log("DecList -> Dec\n");
}   | Dec COMMA DecList {
    $$ = createNode("DecList", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("DecList -> Dec COMMA DecList\n");
}   ;
Dec: VarDec {
    $$ = createNode("Dec", @$.first_line);
    addChild($$, 1, $1);
    Log("Dec -> VarDec\n");
}   | VarDec ASSIGNOP Exp {
    $$ = createNode("Dec", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Dec -> VarDec ASSIGNOP Exp\n");
/* }   | VarDec ASSIGNOP error { */
/*     $$ = createNode("Dec", @$.first_line); */
/*     addChild($$, 3, createNode("Error", @$.first_line), $2, $1); */
/*     synerror("Right side of initial assignment error"); */
/*     if(++errnum >= 10) YYABORT; */
};

Exp: Exp ASSIGNOP Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp ASSIGNOP Exp\n");
}   | Exp AND Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp AND Exp\n");
}   | Exp OR Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp OR Exp\n");
}   | Exp RELOP Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp RELOP Exp\n");
}   | Exp PLUS Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp PLUS Exp\n");
}   | Exp MINUS Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp MINUS Exp\n");
}   | Exp STAR Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp STAR Exp\n");
}   | Exp DIV Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp DIV Exp\n");
}   | LP Exp RP {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> LP Exp RP\n");
}   | MINUS Exp %prec UMINUS {
/* }   | MINUS Exp { */
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("Exp -> MINUS Exp\n");
}   | NOT Exp {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 2, $2, $1);
    Log("Exp -> NOT Exp\n");
}   | ID LP Args RP {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 4, $4, $3, $2, $1);
    Log("Exp -> ID LP Args RP\n");
}   | ID LP RP {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> ID LP RP\n");
}   | Exp LB Exp RB {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 4, $4, $3, $2, $1);
    Log("Exp -> Exp LB Exp RB\n");
/* } */
}   | Exp DOT ID {
    /* | Exp DOT ID { */
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Exp -> Exp DOT ID\n");
}   | ID {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 1, $1);
    Log("Exp -> ID\n");
}   | INT {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 1, $1);
    Log("Exp -> INT\n");
}   | FLOAT {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 1, $1);
    Log("Exp -> FLOAT\n");
/* }   | ID LP error RP { /1* Function call arguments error *1/ */
/*     $$ = createNode("Exp", @$.first_line); */
/*     addChild($$, 4, $4, createNode("Error", @$.first_line), $2, $1); */
/*     Log("Exp -> ID LP error RP\n"); */
/*     synerror("Function call arguments error"); */
    /* yyerrok; */
    /* if(++errnum >= 10) YYABORT; */
/* }   | ID LP error { */
/*     $$ = createNode("Exp", @$.first_line); */
/*     addChild($$, 3, createNode("Error", @$.first_line), $2, $1); */
/*     Log("Exp -> ID LP error\n"); */
/*     synerror("Function call error"); */
/*     /1* yyerrok; *1/ */
/*     if(++errnum >= 10) YYABORT; */
}   | Exp LB error RB { /* array index error */
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 4, $4, createNode("Error", @$.first_line), $2, $1);
    Log("Exp -> Exp LB error RB\n");
    synerror("Array index error");
    /* yyerrok; */
    /* if(++errnum >= 10) YYABORT; */
}   | Exp LB Exp error {
    $$ = createNode("Exp", @$.first_line);
    addChild($$, 4, createNode("Error", @$.first_line), $3, $2, $1);
    Log("Exp -> Exp LB Exp error\n");
    synerror("Missing \"]\"");
    /* if(++errnum >= 10) YYABORT; */
/* }   | Exp ASSIGNOP error { */
/*     $$ = createNode("Exp", @$.first_line); */
/*     addChild($$, 3, createNode("error", @$.first_line), $2, $1); */
/*     synerror("Right side of the assignment error"); */
/*     Log("Exp -> Exp ASSIGNOP error"); */
/*     if(++errnum >= 10) YYABORT; */
}   ;
Args: Exp COMMA Args {
    $$ = createNode("Args", @$.first_line);
    addChild($$, 3, $3, $2, $1);
    Log("Args -> Exp COMMA Args\n");
}   | Exp {
    $$ = createNode("Args", @$.first_line);
    addChild($$, 1, $1);
    Log("Args -> Exp\n");
}   ;

%%
void yyerror(const char* msg) { errnum++; errloc = yylloc; } 
