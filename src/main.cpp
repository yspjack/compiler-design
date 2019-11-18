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
#include "parser.h"
#include "optimize.h"
#include "config.h"
// #define DEBUG

int main() {
    FILE *fin;
    fin = fopen("testfile.txt", "rb");
    //freopen("error.txt", "w", stdout);
#ifndef DEBUG
#endif
    assert(fin);
    int n;

    fseek(fin, 0, SEEK_END);
    n = ftell(fin);
    rewind(fin);

    char *buf = (char *)malloc(n + 1);
    fread(buf, sizeof(char), n, fin);
    fclose(fin);
    buf[n] = '\0';

    initLexer(buf, n);
    nextToken();

    program();
    // sym_table.dump();
    // for (auto &i : ircodes) {
    //     i.dump();
    // }
    void objectCode(const std::vector<IRCode> &ircodes);
#ifdef USE_OPTIMIZE
    vector<IRCode> optCode;
    optimizeIR(ircodes, optCode);
    objectCode(optCode);
#else
    objectCode(ircodes);
#endif
    free(buf);
    return 0;
}