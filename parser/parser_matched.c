/*
Homework:
parser - HW3 PL/0 parser and symbol table

Author(s): Graham Wilson Andrew Vreeland

Language: C only

To Compile:
   gcc -Wall -Wextra -std=c11 -O2 parser.c -o parser

To Execute (on Eustis):
   ./lex <input_file> && ./parser

where:
   <input_file> is the path to a text file holding a PL/0 source program;
   ./lex writes tokens.txt and nametable.txt, and ./parser reads them from
   the current directory and takes no arguments

Notes:
   - Implements the recursive-descent parser and symbol table described in
      the homework instructions.
   - Prints four sections to standard output: Name Table, Parse Trace,
      Symbol Table and Result.
   - Stops at the first syntax or symbol-table error, reports it with its
      token number, and exits with status 1.
   - Exits with status 0 when the whole program is accepted.
   - Tested on Eustis.

Class: COP 3402 - Systems Software

Instructor: Jie Lin, Ph.D.

Due Date: See Webcourses
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//token codes given by the assignment
#define identsym 1
#define numbersym 2
#define plussym 3
#define minussym 4
#define multsym 5
#define slashsym 6
#define eqsym 7
#define neqsym 8
#define lessym 9
#define leqsym 10
#define gtrsym 11
#define geqsym 12
#define lparentsym 13
#define rparentsym 14
#define commasym 15
#define semicolonsym 16
#define periodsym 17
#define assignsym 18
#define initsym 19
#define beginsym 20
#define endsym 21
#define ifsym 22
#define fisym 23
#define thensym 24
#define whilesym 25
#define elihwsym 26
#define dosym 27
#define odsym 28
#define oddsym 29
#define callsym 30
#define constsym 31
#define varsym 32
#define procsym 33
#define writesym 34
#define readsym 35
#define elsesym 36
#define MAX_SYMBOLS 100
#define MAX_NAME 12
#define MAX_DIGITS 6
#define LINE_LEN 256
#define KIND_CONST 1
#define KIND_VAR 2
//spelling of every token, index is the code
const char *spelling[] = {
"", "identifier", "number", "+", "-", "*", "/", "==", "!=", "<",
"<=", ">", ">=", "(", ")", ",", ";", ".", "=", ":=",
"begin", "end", "if", "fi", "then", "while", "elihw", "do", "od", "odd",
"call", "const", "var", "procedure", "write", "read", "else"
};
//error messages, index is the error number
const char *messages[] = {
"",
"the program must end with a period, and nothing may follow it",
"const, var, and read must be followed by an identifier",
"a constant must be declared as name := number",
"declarations must be separated by , and end with ;",
"an identifier that starts a statement must be followed by =",
"begin must be closed with end",
"a condition after if or while must be followed by then",
"if must be closed with fi, and while with elihw",
"a condition must contain one of == != < <= > >=",
"a left parenthesis must be closed with a right parenthesis",
"a factor must be an identifier, a number, or (",
"procedure declarations are not part of this grammar",
"call statements are not part of this grammar",
"identifier already declared in this block",
"undeclared identifier",
"only a variable may be assigned or read",
"symbol table is full",
"cannot open tokens.txt or nametable.txt"
};
typedef struct Token_s {
int code;                       // token code
int index;                      // name table index if its an identifier
char digits[MAX_DIGITS + 2];    // the digits if its a number
} Token;
typedef struct Name_s {
int index;                      // name table index
char name[MAX_NAME + 2];        // identifier name
int line;                       // line it was on
int column;                     // column it was on
} Name;
typedef struct Symbol_s {
int kind;                       // const or var
int name;                       // name table index
int value;                      // value if its a const
int level;                      // lexical level
int addr;                       // address if its a var
int mark;                       // 1 when the block is done
} Symbol;
// Global Variables
Token *tokens = NULL;
int numTokens = 0;
Name *names = NULL;
int numNames = 0;
Symbol table[MAX_SYMBOLS];
int numSymbols = 0;
int nextAddr = 3; //first three spots are SL, DL and RA
int cur = 0; //index of current token
int level = 0;
// Function Prototypes
const char *nameOf(int index);
const char *textOf(Token *t);
void printSymbolTable(void);
void errorAt(int n, int t);
void error(int n);
void symbolError(int n);
int current(void);
void getNextToken(const char *owner);
int lookup(int name);
void checkDeclaration(int name);
int useIdentifier(int name);
void insertConst(int name, int value);
void insertVar(int name);
void program(void);
void block(void);
void constDeclaration(void);
void varDeclaration(void);
void statement(void);
void statementList(void);
void condition(void);
void expression(void);
void term(void);
void factor(void);
void readTokens(FILE *fp);
void readNames(FILE *fp);
int main(void) {
//open both files before printing anything
FILE *tokFile = fopen("tokens.txt", "r");
FILE *nameFile = fopen("nametable.txt", "r");
if(tokFile == NULL || nameFile == NULL){
printf("Name Table:\n\nParse Trace:\n\nSymbol Table:\n\nResult:\n\n");
printf("Error 18: %s\n", messages[18]);
exit(1);
}
readTokens(tokFile);
readNames(nameFile);
fclose(tokFile);
fclose(nameFile);
printf("Name Table:\n\n");
if(numNames > 0){
printf("%-6s %-13s %-5s %s\n", "index", "name", "line", "column");
for(int i = 0; i < numNames; i++){
printf("%-6d %-13s %-5d %d\n", names[i].index, names[i].name, names[i].line, names[i].column);
}
printf("\n");
}
printf("Parse Trace:\n\n");
printf("%-6s %-9s %-13s %s\n", "token", "input", "text", "nonterminal");
program();
printf("\n");
printSymbolTable();
printf("Result:\n\nprogram accepted\n");
free(tokens);
free(names);
return 0;
}
//gets the name from the name table index
const char *nameOf(int index){
for(int i = 0; i < numNames; i++){
if(names[i].index == index){
return names[i].name;
}
}
return "";
}
//gets the text of a token for the trace
const char *textOf(Token *t){
if(t->code == identsym) return nameOf(t->index);
if(t->code == numbersym) return t->digits;
if(t->code > 0 && t->code <= elsesym) return spelling[t->code];
return "";
}
void printSymbolTable(void){
printf("Symbol Table:\n\n");
if(numSymbols == 0) return;
printf("%-6s %-9s %-13s %-6s %-6s %-8s %s\n", "entry", "kind", "name", "value", "level", "address", "mark");
for(int i = 0; i < numSymbols; i++){
char value[16] = "-";
char addr[16] = "-";
//consts get a value, vars get an address
if(table[i].kind == KIND_CONST){
sprintf(value, "%d", table[i].value);
}else{
sprintf(addr, "%d", table[i].addr);
}
printf("%-6d %-9s %-13s %-6s %-6d %-8s %d\n", i, table[i].kind == KIND_CONST ? "constant" : "variable",
nameOf(table[i].name), value, table[i].level, addr, table[i].mark);
}
printf("\n");
}
//prints the error at token t and exits
void errorAt(int n, int t){
//ends the parse trace section
printf("\n");
printSymbolTable();
printf("Result:\n\n");
if(t < numTokens){
printf("Error %d at token %d '%s': %s\n", n, t + 1, textOf(&tokens[t]), messages[n]);
}else{
printf("Error %d at end of tokens: %s\n", n, messages[n]);
}
exit(1);
}
//syntax errors are at the current token
void error(int n){
errorAt(n, cur);
}
//symbol table errors are at the identifier we just used
void symbolError(int n){
errorAt(n, cur - 1);
}
//code of the current token, 0 if we ran out
int current(void){
if(cur < numTokens) return tokens[cur].code;
return 0;
}
//prints the trace line for the current token then moves on
void getNextToken(const char *owner){
Token *t = &tokens[cur];
char input[32];
if(t->code == identsym){
sprintf(input, "%d %d", t->code, t->index);
}else if(t->code == numbersym){
sprintf(input, "%d %s", t->code, t->digits);
}else{
sprintf(input, "%d", t->code);
}
printf("%-6d %-9s %-13s %s\n", cur + 1, input, textOf(t), owner);
cur++;
}
//finds the unmarked entry with this name, -1 if not found
int lookup(int name){
for(int i = numSymbols - 1; i >= 0; i--){
if(table[i].name == name && table[i].mark == 0){
return i;
}
}
return -1;
}
//check for duplicate first then check if table is full
void checkDeclaration(int name){
if(lookup(name) != -1) symbolError(14);
if(numSymbols >= MAX_SYMBOLS) symbolError(17);
}
//looks up an identifier being used and prints the lookup line
int useIdentifier(int name){
int e = lookup(name);
if(e == -1) symbolError(15);
printf("       lookup %s: entry %d, %s, level %d\n", nameOf(name), e,
table[e].kind == KIND_CONST ? "constant" : "variable", table[e].level);
return e;
}
void insertConst(int name, int value){
table[numSymbols].kind = KIND_CONST;
table[numSymbols].name = name;
table[numSymbols].value = value;
table[numSymbols].level = level;
table[numSymbols].addr = 0;
table[numSymbols].mark = 0;
printf("       insert entry %d: constant %s, value %d, level %d\n", numSymbols, nameOf(name), value, level);
numSymbols++;
}
void insertVar(int name){
table[numSymbols].kind = KIND_VAR;
table[numSymbols].name = name;
table[numSymbols].value = 0;
table[numSymbols].level = level;
table[numSymbols].addr = nextAddr;
table[numSymbols].mark = 0;
nextAddr++;
printf("       insert entry %d: variable %s, level %d, address %d\n", numSymbols, nameOf(name), level, table[numSymbols].addr);
numSymbols++;
}
//program -> block "."
void program(void){
block();
if(current() != periodsym) error(1);
getNextToken("program");
//nothing can come after the period
if(cur < numTokens) error(1);
}
//block -> [ const-decl ] [ var-decl ] statement
void block(void){
printf("       enter level %d\n", level);
if(current() == constsym) constDeclaration();
if(current() == varsym) varDeclaration();
statement();
//block is done so mark all its names
for(int i = 0; i < numSymbols; i++){
table[i].mark = 1;
printf("       mark entry %d\n", i);
}
printf("       leave level %d\n", level);
}
//const-decl -> "const" ident ":=" number { "," ident ":=" number } ";"
void constDeclaration(void){
getNextToken("const-decl");
while(1){
if(current() != identsym) error(2);
int name = tokens[cur].index;
getNextToken("const-decl");
checkDeclaration(name);
if(current() != initsym) error(3);
getNextToken("const-decl");
if(current() != numbersym) error(3);
int value = atoi(tokens[cur].digits);
getNextToken("const-decl");
//dont know the value until after the number so insert here
insertConst(name, value);
if(current() != commasym) break;
getNextToken("const-decl");
}
if(current() != semicolonsym) error(4);
getNextToken("const-decl");
}
//var-decl -> "var" ident { "," ident } ";"
void varDeclaration(void){
getNextToken("var-decl");
while(1){
if(current() != identsym) error(2);
int name = tokens[cur].index;
getNextToken("var-decl");
checkDeclaration(name);
insertVar(name);
if(current() != commasym) break;
getNextToken("var-decl");
}
if(current() != semicolonsym) error(4);
getNextToken("var-decl");
}
//statement -> [ ident "=" expression | "begin" statement-list "end" | ... ]
void statement(void){
int e;
switch (current()) {
case identsym:
e = tokens[cur].index;
getNextToken("statement");
//lookup and kind check before the "="
e = useIdentifier(e);
if(table[e].kind != KIND_VAR) symbolError(16);
if(current() != assignsym) error(5);
getNextToken("statement");
expression();
break;
case beginsym:
getNextToken("statement");
statementList();
if(current() != endsym) error(6);
getNextToken("statement");
break;
case ifsym:
getNextToken("statement");
condition();
if(current() != thensym) error(7);
getNextToken("statement");
statementList();
if(current() == elsesym){
getNextToken("statement");
statementList();
}
if(current() != fisym) error(8);
getNextToken("statement");
break;
case whilesym:
getNextToken("statement");
condition();
if(current() != thensym) error(7);
getNextToken("statement");
statementList();
if(current() != elihwsym) error(8);
getNextToken("statement");
break;
case readsym:
getNextToken("statement");
if(current() != identsym) error(2);
e = tokens[cur].index;
getNextToken("statement");
e = useIdentifier(e);
if(table[e].kind != KIND_VAR) symbolError(16);
break;
case writesym:
getNextToken("statement");
expression();
break;
case procsym:
error(12);
break;
case callsym:
error(13);
break;
default: //empty statement
break;
}
}
//statement-list -> statement { ";" statement }
void statementList(void){
statement();
while(current() == semicolonsym){
getNextToken("statement-list");
statement();
}
}
//condition -> expression ( "==" | "!=" | "<" | "<=" | ">" | ">=" ) expression
void condition(void){
expression();
int c = current();
if(c != eqsym && c != neqsym && c != lessym && c != leqsym && c != gtrsym && c != geqsym){
error(9);
}
getNextToken("condition");
expression();
}
//expression -> [ "+" | "-" ] term { ( "+" | "-" ) term }
void expression(void){
if(current() == plussym || current() == minussym){
getNextToken("expression");
}
term();
while(current() == plussym || current() == minussym){
getNextToken("expression");
term();
}
}
//term -> factor { ( "*" | "/" ) factor }
void term(void){
factor();
while(current() == multsym || current() == slashsym){
getNextToken("term");
factor();
}
}
//factor -> ident | number | "(" expression ")"
//the pseudocode had a sign here but the grammar doesnt
void factor(void){
if(current() == identsym){
int name = tokens[cur].index;
getNextToken("factor");
//const or var both work here so no kind check
useIdentifier(name);
}else if(current() == numbersym){
getNextToken("factor");
}else if(current() == lparentsym){
getNextToken("factor");
expression();
if(current() != rparentsym) error(10);
getNextToken("factor");
}else{
error(11);
}
}
//reads tokens.txt into the tokens array
void readTokens(FILE *fp){
char line[LINE_LEN];
int cap = 64;
tokens = malloc(sizeof(Token) * cap);
while(fgets(line, sizeof(line), fp) != NULL){
Token t;
char second[LINE_LEN] = "";
if(sscanf(line, "%d %255s", &t.code, second) < 1) continue;
t.index = -1;
t.digits[0] = '\0';
if(t.code == identsym){
t.index = atoi(second);
}else if(t.code == numbersym){
strncpy(t.digits, second, MAX_DIGITS + 1);
t.digits[MAX_DIGITS + 1] = '\0';
}
//double the array if its full
if(numTokens == cap){
cap *= 2;
tokens = realloc(tokens, sizeof(Token) * cap);
}
tokens[numTokens] = t;
numTokens++;
}
}
//reads nametable.txt into the names array
void readNames(FILE *fp){
char line[LINE_LEN];
int cap = 64;
names = malloc(sizeof(Name) * cap);
while(fgets(line, sizeof(line), fp) != NULL){
Name nm;
char name[LINE_LEN];
if(sscanf(line, "%d %255s %d %d", &nm.index, name, &nm.line, &nm.column) != 4) continue;
strncpy(nm.name, name, MAX_NAME + 1);
nm.name[MAX_NAME + 1] = '\0';
//double the array if its full
if(numNames == cap){
cap *= 2;
names = realloc(names, sizeof(Name) * cap);
}
names[numNames] = nm;
numNames++;
}
}
