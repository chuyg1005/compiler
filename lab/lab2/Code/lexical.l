%{
#include <stdlib.h>
#include "Node.h"
#include "syntax.tab.h"

extern void Log(const char*, ...);    // encapsulate printf
extern void lexerror(int lineno, const char* desc, const char* text);

extern char linebuf[500];
extern int lexerr;

int yycolumn = 1;
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng; 
%}

int     0|[1-9][0-9]*
hex     0[xX][0-9a-fA-F]+
oct     0[0-7]+
float   [0-9]+\.[0-9]+|([0-9]+\.[0-9]*|\.[0-9]+)([Ee][+-]?[0-9]+)
    /* floatErr    ([0-9]+\.[0-9]*|\.[0-9]+)[Ee] */
id      [a-zA-Z_][a-zA-Z0-9_]*
idErr   [0-9][a-zA-Z0-9_]+
relop   ">"|"<"|">="|"<="|"=="|"!="
type    "int"|"float"
    /* ws      [ \t\n] */

    /* don't call yywrap when yylex is over. */
%option noyywrap
%option yylineno
    /* treat no-matched text as error. */
%option nodefault

%x COMMENT

%%
{int}   { 
    Log("int: %d\n", atoi(yytext)); 
    yylval.node = createNode(NODE_INT, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    yylval.node->val.intVal = atoi(yytext);
    return INT;
}
{hex}   {
    lexerror(yylineno, "Hex number is not allowed", yytext);
    yylval.node = createNode(NODE_INT, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    sscanf(yytext, "%x",&(yylval.node->val.intVal));
    /* yylval.node->field->intVal = atoi(yytext); */
    return INT;
}
{oct}   {
    lexerror(yylineno, "Oct number is not allowed", yytext);
    yylval.node = createNode(NODE_INT, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    sscanf(yytext, "%o",&(yylval.node->val.intVal));
    return INT;
}
{float} { 
    Log("float: %f\n", atof(yytext)); 
    yylval.node = createNode(NODE_FLOAT, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    yylval.node->val.floatVal = atof(yytext);
    return FLOAT;
}
";"     { 
    Log("semicolon\n"); 
    yylval.node = createNode(NODE_SEMI, yylineno);
    return SEMI;
}
","     { 
    Log("comma\n"); 
    yylval.node = createNode(NODE_COMMA, yylineno);
    return COMMA;
}
"="     { 
    Log("assignment\n"); 
    yylval.node = createNode(NODE_ASSIGNOP, yylineno);
    return ASSIGNOP;
}
{relop} { 
    Log("relop\n"); 
    yylval.node = createNode(NODE_RELOP, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    /* yylval.node->field->string = (char*)malloc(sizeof(char)*(yyleng + 1)); */
    /* memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1)); */
    strncpy(yylval.node->val.name, yytext, yyleng);
    return RELOP;
}
"+"     { 
    Log("plus\n"); 
    yylval.node = createNode(NODE_PLUS, yylineno);
    return PLUS;
}
"-"     { 
    Log("minus\n"); 
    yylval.node = createNode(NODE_MINUS, yylineno);
    return MINUS;
}
"*"     { 
    Log("star\n"); 
    yylval.node = createNode(NODE_STAR, yylineno);
    return STAR;
}
"/"     { 
    Log("divide\n"); 
    yylval.node = createNode(NODE_DIV, yylineno);
    return DIV;
}
"&&"    { 
    Log("and\n"); 
    yylval.node = createNode(NODE_AND, yylineno);
    return AND;
}
"||"    { 
    Log("or\n"); 
    yylval.node = createNode(NODE_OR, yylineno);
    return OR;
}
"."     { 
    Log("dot\n"); 
    yylval.node = createNode(NODE_DOT, yylineno);
    return DOT;
}
"!"     { 
    Log("not\n"); 
    yylval.node = createNode(NODE_NOT, yylineno);
    return NOT;
}
{type}  { 
    Log("type: %s\n", yytext); 
    yylval.node = createNode(NODE_TYPE, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    /* yylval.node->field->string = (char*)malloc(sizeof(char)*(yyleng + 1)); */
    /* memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1)); */
    strncpy(yylval.node->val.name, yytext, yyleng);
    return TYPE;
}
"("     { 
    Log("lp\n"); 
    yylval.node = createNode(NODE_LP, yylineno);
    return LP;
}
")"     { 
    Log("rp\n"); 
    yylval.node = createNode(NODE_RP, yylineno);
    return RP;
}
"["     { 
    Log("lb\n"); 
    yylval.node = createNode(NODE_LB, yylineno);
    return LB;
}
"]"     { 
    Log("rb\n"); 
    yylval.node = createNode(NODE_RB, yylineno);
    return RB;
}
"{"     { 
    Log("lc\n"); 
    yylval.node = createNode(NODE_LC, yylineno);
    return LC;
}
"}"     { 
    Log("rc\n"); 
    yylval.node = createNode(NODE_RC, yylineno);
    return RC;
}
"if"    { 
    Log("if\n"); 
    yylval.node = createNode(NODE_IF, yylineno);
    return IF;
}
"else"  { 
    Log("else\n"); 
    yylval.node = createNode(NODE_ELSE, yylineno);
    return ELSE;
}
"while" { 
    Log("while\n"); 
    yylval.node = createNode(NODE_WHILE, yylineno);
    return WHILE;
}
"struct"    { 
    Log("struct\n"); 
    yylval.node = createNode(NODE_STRUCT, yylineno);
    return STRUCT;
}
"return"    { 
    Log("return\n"); 
    yylval.node = createNode(NODE_RETURN, yylineno);
    return RETURN;
}
"//".*    { lexerror(yylineno, "Single-Line Comment not support", yytext); }
"/*"    { 
    lexerror(yylineno, "Multi-Line Comment not support, begin this line", NULL); 
    BEGIN(COMMENT); 
}
<COMMENT>"*/"   {
    lexerror(yylineno, "Multi-Line Comment not support, end this line", NULL);
    BEGIN(INITIAL);
}
<COMMENT>.|\n   { /* comment state */ }
<COMMENT><<EOF>>    {
    lexerror(yylineno, "Multi-Line Comment not support, comment not end..", NULL);
    BEGIN(INITIAL);
}
{id}    { 
    Log("id: %s\n", yytext); 
    yylval.node = createNode(NODE_ID, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    /* yylval.node->field->string = (char*)malloc(sizeof(char) * (yyleng+1)); */
    /* memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1)); */
    strncpy(yylval.node->val.name, yytext, yyleng);
    return ID;
}
    /* {floatErr}  { */ 
    /*     lexerr = 1; */
    /*     lexerror(yylineno, "Illegal floating point number", yytext); */ 
    /*     return FLOAT; */
    /* } */
{idErr} { 
    lexerr = 1;
    lexerror(yylineno, "Illegal identifier started with number", yytext); 
    yylval.node = createNode(NODE_ID, yylineno);
    /* yylval.node->field = (Field*)malloc(sizeof(Field)); */
    /* memset(yylval.node->field, 0, sizeof(Field)); */
    /* yylval.node->field->string = (char*)malloc(sizeof(char) * (yyleng+1)); */
    /* memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1)); */
    strncpy(yylval.node->val.name, yytext, yyleng);
    return ID;
}
[ \t]+   {  }
(\r\n|\n).*    { 
    yycolumn = 1; 
    strncpy(linebuf, yytext+1, yyleng);
    yyless(1);
}
.       { 
    lexerr = 1;
    lexerror(yylineno, "Mysterious character", yytext); 
}

%%
