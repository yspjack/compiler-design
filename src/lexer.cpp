#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cassert>

#include "lexer.h"
#include "errproc.h"

const char* TokenTypeString[] = {
    "UKN",
    "IDENFR",
    "INTCON",
    "CHARCON",
    "STRCON",
    "CONSTTK",
    "INTTK",
    "CHARTK",
    "VOIDTK",
    "MAINTK",
    "IFTK",
    "ELSETK",
    "DOTK",
    "WHILETK",
    "FORTK",
    "SCANFTK",
    "PRINTFTK",
    "RETURNTK",
    "PLUS",
    "MINU",
    "MULT",
    "DIV",
    "LSS",
    "LEQ",
    "GRE",
    "GEQ",
    "EQL",
    "NEQ",
    "ASSIGN",
    "SEMICN",
    "COMMA",
    "LPARENT",
    "RPARENT",
    "LBRACK",
    "RBRACK",
    "LBRACE",
    "RBRACE",
    "END",
    "ILLCHAR"
};
using namespace std;

int isLetter(char c) {
    return isalpha(c) || c == '_';
}

static int n;
static char* buf;

int tokenType;
string tokenVal;

// position in buffer
int cur;

// Line number for previous token
int lastline;

// Line number for current token
int linenumber;

void initLexer(char* str, int len) {
    buf = str;
    n = len;
    cur = 0;
    lastline = 1;
    linenumber = 1;
}

int lookAhead(int n)
{
    if (n < 1) {
        return UKN;
    }
    int old_cur = cur;
    int old_lastline = lastline;
    int old_linenumber = linenumber;
    int old_tokenType = tokenType;
    string old_tokenVal = tokenVal;

    int result;

    for (int i = 0; i < n; ++i) {
        nextToken();
    }
    result = tokenType;
    cur = old_cur;
    lastline = old_lastline;
    linenumber = old_linenumber;
    tokenType = old_tokenType;
    tokenVal = old_tokenVal;
    return result;
}

bool isLegalChar(char ch)
{
    return isdigit(ch) || isalpha(ch) || ch == '_' || ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

void nextToken()
{
    lastline = linenumber;
    while (cur < n && isspace(buf[cur]))
    {
        if (buf[cur] == '\n') {
            ++linenumber;
        }
        ++cur;
    }
    if (cur >= n)
    {
        tokenType = END;
        return;
    }
    if (isLetter(buf[cur])) {
        tokenVal = buf[cur];
        ++cur;
        while (cur < n && (isLetter(buf[cur]) || isdigit(buf[cur]))) {
            tokenVal += buf[cur];
            ++cur;
        }
        if (tokenVal == "const") { tokenType = CONSTTK; }
        else if (tokenVal == "int") { tokenType = INTTK; }
        else if (tokenVal == "char") { tokenType = CHARTK; }
        else if (tokenVal == "void") { tokenType = VOIDTK; }
        else if (tokenVal == "main") { tokenType = MAINTK; }
        else if (tokenVal == "if") { tokenType = IFTK; }
        else if (tokenVal == "else") { tokenType = ELSETK; }
        else if (tokenVal == "do") { tokenType = DOTK; }
        else if (tokenVal == "while") { tokenType = WHILETK; }
        else if (tokenVal == "for") { tokenType = FORTK; }
        else if (tokenVal == "scanf") { tokenType = SCANFTK; }
        else if (tokenVal == "printf") { tokenType = PRINTFTK; }
        else if (tokenVal == "return") { tokenType = RETURNTK; }
        else { tokenType = IDENFR; }
    }
    else if (isdigit(buf[cur])) {
        tokenType = INTCON;
        tokenVal = buf[cur];
        ++cur;
        while (cur < n && isdigit(buf[cur])) {
            tokenVal += buf[cur];
            ++cur;
        }
        //tokenIntVal = atoi(tokenVal.c_str());
        if (tokenVal.length() > 1 && tokenVal[0] == '0') {
            handleError(ILLEGAL_SYMBOL, linenumber);
        }
    }
    else if (buf[cur] == '\'') {
        if (cur + 2 < n && buf[cur + 2] == '\'') {
            if (isLegalChar(buf[cur + 1])) {
                tokenType = CHARCON;
                tokenVal = buf[cur + 1];
                //tokenCharVal = buf[cur + 1];
                cur += 3;
            }
            else {
                //tokenType = ILLCHAR;
                tokenType = CHARCON;
                handleError(ILLEGAL_SYMBOL, linenumber);
                cur += 3;
            }
        }
    }
    else if (buf[cur] == '\"') {
        ++cur;
        tokenType = STRCON;
        tokenVal = "";
        while (cur < n && buf[cur] != '\"') {
            tokenVal += buf[cur];
            ++cur;
        }
        ++cur;

        for (char c : tokenVal) {
            if (!(c == 32 || c == 33 || (c >= 35 && c <= 126))) {
                handleError(ILLEGAL_SYMBOL, linenumber);
                break;
            }
        }
    }
    else if (buf[cur] == '+') { tokenType = PLUS; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '-') { tokenType = MINU; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '*') { tokenType = MULT; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '/') { tokenType = DIV; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == ';') { tokenType = SEMICN; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == ',') { tokenType = COMMA; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '(') { tokenType = LPARENT; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == ')') { tokenType = RPARENT; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '[') { tokenType = LBRACK; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == ']') { tokenType = RBRACK; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '{') { tokenType = LBRACE; tokenVal = buf[cur]; ++cur; }
    else if (buf[cur] == '}') {
        tokenType = RBRACE; tokenVal = buf[cur]; ++cur;
    }
    else if (buf[cur] == '>') {
        tokenVal = buf[cur];
        ++cur;
        if (cur < n && buf[cur] == '=') {
            tokenType = GEQ;
            tokenVal += buf[cur];
            ++cur;
        }
        else {
            tokenType = GRE;
        }
    }
    else if (buf[cur] == '<') {
        tokenVal = buf[cur];
        ++cur;
        if (cur < n && buf[cur] == '=') {
            tokenVal += buf[cur];
            tokenType = LEQ;
            ++cur;
        }
        else {
            tokenType = LSS;
        }
    }
    else if (buf[cur] == '!') {
        ++cur;
        if (cur < n && buf[cur] == '=') {
            tokenVal = "!=";
            tokenType = NEQ;
            ++cur;

        }
    }
    else if (buf[cur] == '=') {
        tokenVal = buf[cur];
        ++cur;
        if (cur < n && buf[cur] == '=') {
            tokenVal += buf[cur];
            tokenType = EQL;
            ++cur;

        }
        else {
            tokenType = ASSIGN;
        }
    }
    else {
        tokenType = UKN;
        printf("Unknown char %c\n", buf[cur]);
        ++cur;
    }
}

