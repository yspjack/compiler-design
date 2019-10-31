#ifndef LEXER_H
#define LEXER_H
#include <string>
using std::string;

enum TOKENTYPE
{
    UKN = 0,
    IDENFR,
    INTCON,
    CHARCON,
    STRCON,
    CONSTTK,
    INTTK,
    CHARTK,
    VOIDTK,
    MAINTK,
    IFTK,
    ELSETK,
    DOTK,
    WHILETK,
    FORTK,
    SCANFTK,
    PRINTFTK,
    RETURNTK,
    PLUS,
    MINU,
    MULT,
    DIV,
    LSS,
    LEQ,
    GRE,
    GEQ,
    EQL,
    NEQ,
    ASSIGN,
    SEMICN,
    COMMA,
    LPARENT,
    RPARENT,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,
    END,
    ILLCHAR
};

extern const char* TokenTypeString[];
extern int tokenType;
extern string tokenVal;
extern int lastline;
extern int linenumber;
extern void initLexer(char* str, int len);
extern void nextToken();
extern int lookAhead(int n);

#endif // !LEXER_H

