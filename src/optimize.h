#if !defined(OPTIMIZE_H)
#define OPTIMIZE_H
#include <vector>
#include "IR.h"

std::vector<IRCode> algebraOptimize(const std::vector<IRCode> &ircodes);

#endif // OPTIMIZE_H
