#include <cstdio>
#include "lexer.h"
#include "errproc.h"
const char* errString[] = {
    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
    "g",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "o"
};

void handleError(int errCode,int line)
{
    FOUND_ERR = 1;
    if (errCode < 15)
        fprintf(err_fout, "%d %s\n", line, errString[errCode]);
    else
        fprintf(err_fout, "%d %d\n", line, errCode);
}