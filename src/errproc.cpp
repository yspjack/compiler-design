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
    if (errCode < 15)
        printf("%d %s\n", line, errString[errCode]);
    else
        printf("%d %d\n", line, errCode);
}