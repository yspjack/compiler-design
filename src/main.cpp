#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
//#define NDEBUG
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include "config.h"
#include "lexer.h"
#include "parser.h"
#include "optimize.h"
#include "errproc.h"
// #define DEBUG

int FOUND_ERR = 0;
FILE* err_fout = NULL;

int main() {
    FILE *fin;
    fin = fopen("testfile.txt", "rb");
    err_fout = fopen("error.txt", "w");
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
    fclose(err_fout);
    if (FOUND_ERR) {
        free(buf);
        return 0;
    }
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