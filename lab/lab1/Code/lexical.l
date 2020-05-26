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
    yylval.node = createNode("INT", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->intVal = atoi(yytext);
    return INT;
}
{hex}   {
    lexerror(yylineno, "Hex number is not allowed", yytext);
    yylval.node = createNode("INT", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    sscanf(yytext, "%x",&(yylval.node->field->intVal));
    /* yylval.node->field->intVal = atoi(yytext); */
    return INT;
}
{oct}   {
    lexerror(yylineno, "Oct number is not allowed", yytext);
    yylval.node = createNode("INT", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    sscanf(yytext, "%o",&(yylval.node->field->intVal));
    return INT;
}
{float} { 
    Log("float: %f\n", atof(yytext)); 
    yylval.node = createNode("FLOAT", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->floatVal = atof(yytext);
    return FLOAT;
}
";"     { 
    Log("semicolon\n"); 
    yylval.node = createNode("SEMI", yylineno);
    return SEMI;
}
","     { 
    Log("comma\n"); 
    yylval.node = createNode("COMMA", yylineno);
    return COMMA;
}
"="     { 
    Log("assignment\n"); 
    yylval.node = createNode("ASSIGNOP", yylineno);
    return ASSIGNOP;
}
{relop} { 
    Log("relop\n"); 
    yylval.node = createNode("RELOP", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->string = (char*)malloc(sizeof(char)*(yyleng + 1));
    memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1));
    strncpy(yylval.node->field->string, yytext, yyleng);
    return RELOP;
}
"+"     { 
    Log("plus\n"); 
    yylval.node = createNode("PLUS", yylineno);
    return PLUS;
}
"-"     { 
    Log("minus\n"); 
    yylval.node = createNode("MINUS", yylineno);
    return MINUS;
}
"*"     { 
    Log("star\n"); 
    yylval.node = createNode("STAR", yylineno);
    return STAR;
}
"/"     { 
    Log("divide\n"); 
    yylval.node = createNode("DIV", yylineno);
    return DIV;
}
"&&"    { 
    Log("and\n"); 
    yylval.node = createNode("AND", yylineno);
    return AND;
}
"||"    { 
    Log("or\n"); 
    yylval.node = createNode("OR", yylineno);
    return OR;
}
"."     { 
    Log("dot\n"); 
    yylval.node = createNode("DOT", yylineno);
    return DOT;
}
"!"     { 
    Log("not\n"); 
    yylval.node = createNode("NOT", yylineno);
    return NOT;
}
{type}  { 
    Log("type: %s\n", yytext); 
    yylval.node = createNode("TYPE", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->string = (char*)malloc(sizeof(char)*(yyleng + 1));
    memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1));
    strncpy(yylval.node->field->string, yytext, yyleng);
    return TYPE;
}
"("     { 
    Log("lp\n"); 
    yylval.node = createNode("LP", yylineno);
    return LP;
}
")"     { 
    Log("rp\n"); 
    yylval.node = createNode("RP", yylineno);
    return RP;
}
"["     { 
    Log("lb\n"); 
    yylval.node = createNode("LB", yylineno);
    return LB;
}
"]"     { 
    Log("rb\n"); 
    yylval.node = createNode("RB", yylineno);
    return RB;
}
"{"     { 
    Log("lc\n"); 
    yylval.node = createNode("LC", yylineno);
    return LC;
}
"}"     { 
    Log("rc\n"); 
    yylval.node = createNode("RC", yylineno);
    return RC;
}
"if"    { 
    Log("if\n"); 
    yylval.node = createNode("IF", yylineno);
    return IF;
}
"else"  { 
    Log("else\n"); 
    yylval.node = createNode("ELSE", yylineno);
    return ELSE;
}
"while" { 
    Log("while\n"); 
    yylval.node = createNode("WHILE", yylineno);
    return WHILE;
}
"struct"    { 
    Log("struct\n"); 
    yylval.node = createNode("STRUCT", yylineno);
    return STRUCT;
}
"return"    { 
    Log("return\n"); 
    yylval.node = createNode("RETURN", yylineno);
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
    yylval.node = createNode("ID", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->string = (char*)malloc(sizeof(char) * (yyleng+1));
    memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1));
    strncpy(yylval.node->field->string, yytext, yyleng);
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
    yylval.node = createNode("ID", yylineno);
    yylval.node->field = (Field*)malloc(sizeof(Field));
    memset(yylval.node->field, 0, sizeof(Field));
    yylval.node->field->string = (char*)malloc(sizeof(char) * (yyleng+1));
    memset(yylval.node->field->string, 0, sizeof(char)*(yyleng+1));
    strncpy(yylval.node->field->string, yytext, yyleng);
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
