#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "Node.h"
#include "syntax.tab.h"

#ifdef YYDEBUG
int yydebug = 1;
#endif

extern void yyrestart(FILE*);
extern int yyparse();
extern int yylineno;

Node* root = NULL;
int errnum = 0;
int lexerr = 0;
int lastline = -1;
bool output = false;
char linebuf[4096];
char filename[128];
YYLTYPE errloc;
void synerror(const char*);
void lexerror(int, const char*, const char*);

int main(int argc, char**argv) {
    if(argc < 2) return 1;
    FILE* f = fopen(argv[1], "r");
    if(!f) {
        perror(argv[1]);
        return 1;
    }
    strncpy(filename, argv[1], strlen(argv[1])+1);
    fgets(linebuf, sizeof(linebuf), f);
    linebuf[strlen(linebuf)-1] = '\0';
    fseek(f, 0L, SEEK_SET);
    yyrestart(f);
    yyparse();
    // no lexical error and no syntax error
    if(errnum == 0 && !lexerr) {
        /* printf("syntax analyze succeed.\n"); */
        traverseTree(root, 0);
    } else if(!output && errnum > 0){ /* synchronized token not found */
        fprintf(stderr, "\033[31mError type B at line %d: Syntax error.\n\033[0m", 
                errloc.first_line);
    }
    freeTree(root);
    fclose(f);
}
void synerror(const char* msg) {
    output = true;
    /* errnum++; */
    if(errloc.first_line != lastline){
        if(errloc.first_line == yylineno) {
            fprintf(stderr, "\033[31mError type B at line %d: %s.\t\033[33m[%s:%d:%d: %s]\n\033[0m", 
                    errloc.first_line, msg, filename, errloc.first_line, errloc.first_column, linebuf);
        } else {
            fprintf(stderr, "\033[31mError type B at line %d: %s.\n\033[0m", 
                    errloc.first_line, msg);
        }
        /* lastline = yylineno; */
        lastline = errloc.first_line;
    }
}
void lexerror(int lineno, const char* desc, const char* text) {
    lexerr = 1;
    /* errnum++; */
    if(text != NULL) {
        fprintf(stderr, "\033[31mError type A at line %d: %s \"%s\".\t\033[33m[%s:%d:%d: %s]\n\033[0m", 
                lineno, desc, text, filename, yylineno, yylloc.first_column, linebuf);
    } else {
        fprintf(stderr, "\033[31mError type A at line %d: %s.\t\033[33m[%s:%d:%d: %s]\n\033[0m", 
                lineno, desc, filename, yylineno, yylloc.first_column, linebuf);
    }
    lastline = yylineno;
}
void Log(const char* format, ...) {
#ifdef DEBUG
    printf("\033[33mdebug mode> ");
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\033[0m");
#endif
}
