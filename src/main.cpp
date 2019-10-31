#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
//#define NDEBUG
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include "lexer.h"

extern void program();
#define DEBUG


int main() {
    FILE* fin;
    fin = fopen("testfile.txt", "rb");
#ifndef DEBUG
    freopen("error.txt", "w", stdout);
#endif
    assert(fin);
    int n;

    fseek(fin, 0, SEEK_END);
    n = ftell(fin);
    rewind(fin);

    char* buf = (char*)malloc(n + 1);
    fread(buf, sizeof(char), n, fin);
    fclose(fin);
    buf[n] = '\0';

    initLexer(buf, n);
    nextToken();

    program();
    free(buf);
    return 0;
}