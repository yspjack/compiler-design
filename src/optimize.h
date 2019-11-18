#if !defined(OPTIMIZE_H)
#define OPTIMIZE_H
#include <vector>
#include "IR.h"

void optimizeIR(const std::vector<IRCode> &ircodes,
                     std::vector<IRCode> &optCode);
#endif // OPTIMIZE_H
