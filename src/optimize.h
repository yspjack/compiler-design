#if !defined(OPTIMIZE_H)
#define OPTIMIZE_H
#include <vector>
#include <map>
#include <string>
#include "IR.h"

struct FunctionBlock {
    std::string func;
    std::vector<IRCode> ircodes;
    std::vector<std::vector<IRCode>> blocks;
    std::map<int, std::vector<int>> blockNext;
    std::map<std::string, int> liveVarCount;
    // const vector<IRCode> &operator[](size_t idx) const { return blocks[idx];
    // } vector<IRCode> &operator[](size_t idx) { return blocks[idx]; }
};

void optimizeIR(const std::vector<IRCode> &ircodes,
                     std::vector<IRCode> &optCode);
void liveDataflow(FunctionBlock &funcBlock);
#endif // OPTIMIZE_H
