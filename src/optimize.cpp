#include "config.h"
#ifdef USE_OPTIMIZE
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include <set>
#include <iostream>
#include <queue>
#include <stack>
#include <iterator>
#include "IR.h"
#include "parser.h"
#include "optimize.h"
// using std::map;
// using std::set;
// using std::string;
// using std::stringstream;
// using std::to_string;
// using std::vector;
using namespace std;

void algebraOptimize(const vector<IRCode>& ircodes, vector<IRCode>& result) {
    for (const auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
            if (ircode.op1 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op2, "", ircode.dst));
            }
            else if (ircode.op2 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            }
            else {
                result.push_back(ircode);
            }
            break;
        case IROperator::SUB:
            if (ircode.op1 == ircode.op2) {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            }
            else if (ircode.op2 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            }
            else {
                result.push_back(ircode);
            }
            break;
        case IROperator::MUL:
            if (ircode.op1 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op2, "", ircode.dst));
            }
            else if (ircode.op2 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            }
            else if (ircode.op1 == "0" || ircode.op2 == "0") {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            }
            else {
                result.push_back(ircode);
            }
            break;
        case IROperator::DIV:
            if (ircode.op1 == ircode.op2) {
                result.push_back(IRCode(IROperator::MOV, "1", "", ircode.dst));
            }
            else if (ircode.op2 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            }
            else if (ircode.op1 == "0" || ircode.op2 == "0") {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            }
            else {
                result.push_back(ircode);
            }
            break;
        default:
            result.push_back(ircode);
            break;
        }
    }
}

bool isConst(const string& func, const string& var) {
    Symbol* s = sym_table.getByName(func, var);
    if (s != nullptr && s->clazz == Symbol::SYM_CONST) {
        return true;
    }
    if (var.length() < 1) {
        return false;
    }
    int i = 0;
    if (var[0] == '-' || var[0] == '+') {
        i = 1;
    }
    else {
        i = 0;
    }
    for (; i < var.length(); i++) {
        if (!isdigit(var[i])) {
            return false;
        }
    }
    return true;
}

bool getConst(const string& func, const string& var, int& value) {
    Symbol* s = sym_table.getByName(func, var);
    if (s != nullptr && s->clazz == Symbol::SYM_CONST) {
        value = s->value;
        return true;
    }
    if (var.length() < 1) {
        return false;
    }
    int i = 0;
    if (var[0] == '-' || var[0] == '+') {
        i = 1;
    }
    else {
        i = 0;
    }
    for (; i < var.length(); i++) {
        if (!isdigit(var[i])) {
            return false;
        }
    }
    value = stoi(var);
    return true;
}

map<string, int> crossBlockTmp(FunctionBlock& local) {
    auto& func = local.func;
    auto& blocks = local.blocks;
    auto& ircodes = local.ircodes;
    vector<set<string>> useDefTmp;
    auto addTmp = [&](set<string>& s, const string& name) {
        if (name.length() > 2 && name[0] == '#' && name[1] == 't')
        {
            s.insert(name);
        }
    };

    if (func == "") {
        return map<string, int>();
    }

    for (int i = 0; i < blocks.size(); i++) {
        useDefTmp.push_back(set<string>());
        auto& tmpList = useDefTmp[i];
        for (const auto& ircode : blocks[i]) {
            switch (ircode.op) {
            case IROperator::ADD:
            case IROperator::SUB:
            case IROperator::MUL:
            case IROperator::DIV:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.op2);
                addTmp(tmpList, ircode.dst);
                break;
            case IROperator::LEQ:
            case IROperator::LT:
            case IROperator::GEQ:
            case IROperator::GT:
            case IROperator::NEQ:
            case IROperator::EQU:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.op2);
                addTmp(tmpList, ircode.dst);
                break;
            case IROperator::READ:
            case IROperator::WRITE:
                addTmp(tmpList, ircode.op1);
                break;
            case IROperator::PUSH:
                addTmp(tmpList, ircode.op1);
                break;
            case IROperator::RET:
                if (ircode.op1 != "") {
                    addTmp(tmpList, ircode.op1);
                }
                break;
            case IROperator::LOADARR:
            case IROperator::SAVEARR:
                addTmp(tmpList, ircode.op2);
                addTmp(tmpList, ircode.dst);
                break;
            case IROperator::BZ:
            case IROperator::BNZ:
                addTmp(tmpList, ircode.op1);
                break;
            case IROperator::MOV:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.dst);
                break;
            default:
                break;
            }
        }
    }

    set<string> tmpList;
    for (const auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.op2);
            tmpList.insert(ircode.dst);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.op2);
            tmpList.insert(ircode.dst);
            break;
        case IROperator::READ:
        case IROperator::WRITE:
            tmpList.insert(ircode.op1);
            break;
        case IROperator::PUSH:
            tmpList.insert(ircode.op1);
            break;
        case IROperator::RET:
            if (ircode.op1 != "") {
                tmpList.insert(ircode.op1);
            }
            break;
        case IROperator::LOADARR:
        case IROperator::SAVEARR:
            tmpList.insert(ircode.op2);
            tmpList.insert(ircode.dst);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            tmpList.insert(ircode.op1);
            break;
        case IROperator::MOV:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.dst);
            break;
        default:
            break;
        }
    }

    for (auto it = tmpList.begin(); it != tmpList.end();)
    {
        if ((*it).substr(0, 2) == "#t") {
            ++it;
        }
        else {
            it = tmpList.erase(it);
        }
    }

    map<string, int> cnt;
    for (const string& t : tmpList) {
        for (int i = 0; i < useDefTmp.size(); i++) {
            if (useDefTmp[i].count(t)) {
                cnt[t]++;
            }
        }
    }
    return cnt;
};

#ifdef OPT_CONST
void constCombine(const string& func, const vector<IRCode>& ircodes,
    vector<IRCode>& result) {
    int a, b;
    bool c1, c2;

    for (const IRCode& code : ircodes) {
        c1 = getConst(func, code.op1, a);
        c2 = getConst(func, code.op2, b);
        switch (code.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            if (c1 && c2) {
                switch (code.op) {
                case IROperator::ADD:
                    result.emplace_back(IRCode(IROperator::MOV,
                        to_string(a + b), "", code.dst));
                    break;
                case IROperator::SUB:
                    result.emplace_back(IRCode(IROperator::MOV,
                        to_string(a - b), "", code.dst));
                    break;
                case IROperator::MUL:
                    result.emplace_back(IRCode(IROperator::MOV,
                        to_string(a * b), "", code.dst));
                    break;
                case IROperator::DIV:
                    result.emplace_back(IRCode(IROperator::MOV,
                        to_string(b == 0 ? 0 : (a / b)),
                        "", code.dst));
                    break;
                    //case IROperator::LEQ:
                    //    result.emplace_back(IRCode(
                    //        IROperator::MOV, to_string(a <= b), "", code.dst));
                    //    break;
                    //case IROperator::LT:
                    //    result.emplace_back(IRCode(IROperator::MOV,
                    //        to_string(a < b), "", code.dst));
                    //    break;
                    //case IROperator::GEQ:
                    //    result.emplace_back(IRCode(
                    //        IROperator::MOV, to_string(a >= b), "", code.dst));
                    //    break;
                    //case IROperator::GT:
                    //    result.emplace_back(IRCode(IROperator::MOV,
                    //        to_string(a > b), "", code.dst));
                    //    break;
                    //case IROperator::NEQ:
                    //    result.emplace_back(IRCode(
                    //        IROperator::MOV, to_string(a != b), "", code.dst));
                    //    break;
                    //case IROperator::EQU:
                    //    result.emplace_back(IRCode(
                    //        IROperator::MOV, to_string(a == b), "", code.dst));
                    //    break;
                default:
                    result.push_back(code);
                    break;
                }
            }
            else {
                result.push_back(code);
            }
            break;
        default:
            result.push_back(code);
            break;
        }
    }
    // for (auto &code : result) {
    //     code.dump();
    // }
}

void constSpread(FunctionBlock& local) {

    map<string, int> cnt = crossBlockTmp(local);
    auto& func = local.func;
    auto& blocks = local.blocks;
    auto& ircodes = local.ircodes;
    vector<IRCode> result;
    // Find constant
    map<string, string> tmpConst;
    auto rewriteConst = [&](IRCode& ircode) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            if (tmpConst.count(ircode.op2)) {
                ircode.op2 = tmpConst[ircode.op2];
            }
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            if (tmpConst.count(ircode.op2)) {
                ircode.op2 = tmpConst[ircode.op2];
            }
            break;
        case IROperator::WRITE:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            break;
        case IROperator::PUSH:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            break;
        case IROperator::RET:
            if (ircode.op1 != "") {
                if (tmpConst.count(ircode.op1)) {
                    ircode.op1 = tmpConst[ircode.op1];
                }
            }
            break;
        case IROperator::LOADARR:
            if (tmpConst.count(ircode.op2)) {
                ircode.op2 = tmpConst[ircode.op2];
            }
            break;
        case IROperator::SAVEARR:
            if (tmpConst.count(ircode.op2)) {
                ircode.op2 = tmpConst[ircode.op2];
            }
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            break;
        case IROperator::MOV:
            if (tmpConst.count(ircode.op1)) {
                ircode.op1 = tmpConst[ircode.op1];
            }
            break;
        default:
            break;
        }
    };
    for (auto& blk : blocks) {
        vector<IRCode> tmpBlock;
        for (auto& code : blk) {
            bool c1 = false, c2 = false;
            int a, b;
            rewriteConst(code);
            if ((code.dst.length() > 2 && code.dst.substr(0, 2) == "#t")) {
                switch (code.op) {
                case IROperator::ADD:
                case IROperator::SUB:
                case IROperator::MUL:
                case IROperator::DIV:
                    c1 = getConst(func, code.op1, a);
                    c2 = getConst(func, code.op2, b);
                    if (c1 && c2 && cnt[code.dst] <= 1) {
                        switch (code.op) {
                        case IROperator::ADD:
                            tmpConst[code.dst] = to_string(a + b);
                            break;
                        case IROperator::SUB:
                            tmpConst[code.dst] = to_string(a - b);
                            break;
                        case IROperator::MUL:
                            tmpConst[code.dst] = to_string(a * b);
                            break;
                        case IROperator::DIV:
                            tmpConst[code.dst] = to_string(a / b);
                            break;
                        default:
                            assert(0);
                            break;
                        }
                    }
                    else {
                        result.push_back(code);
                        tmpBlock.push_back(code);
                    }
                    break;
                case IROperator::MOV:
                    c1 = getConst(func, code.op1, a);
                    if (c1 && cnt[code.dst] <= 1) {
                        tmpConst[code.dst] = to_string(a);
                    }
                    else {
                        result.push_back(code);
                        tmpBlock.push_back(code);
                    }
                    break;
                default:
                    result.push_back(code);
                    tmpBlock.push_back(code);
                    break;
                }
            }
            else {
                result.push_back(code);
                tmpBlock.push_back(code);
            }
        }
        blk = tmpBlock;
    }
#ifdef DEBUG
    //printf("--CONST SPREAD\n");
    //for (auto &code : result) {
    //    code.dump();
    //}
#endif // DEBUG

    local.ircodes = result;
}
#endif

#ifdef OPT_DAG

void optDag(FunctionBlock& local) {
#ifdef DEBUG
    printf("--DAG(%s):\n", local.func.c_str());
#endif
    map<string, int> cnt = crossBlockTmp(local);
    vector<IRCode> result;

    for (auto& block : local.blocks) {
        vector<IRCode> tmpBlock;
        for (int i = 0; i < block.size(); i++) {
            const IRCode& code = block[i];
            // Scan reusable code
            IRCode* reusableIR = nullptr;
            int j;
            for (j = i - 1; j >= 0; j--)
            {
                IRCode& ircode = block[j];
                if (ircode.op == code.op) {
                    switch (ircode.op) {
                    case IROperator::ADD:
                    case IROperator::SUB:
                    case IROperator::MUL:
                    case IROperator::DIV:
                        if (ircode.op1 == code.op1 && ircode.op2 == code.op2) {
                            reusableIR = &ircode;
                        }
                        break;
                    case IROperator::MOV:
                        if (ircode.op1 == code.op1 && code.op1 != "#RET") {
                            reusableIR = &ircode;
                        }
                        break;
                    default:
                        break;
                    }
                }
                if (reusableIR != nullptr) {
                    break;
                }
            }
            // No define before
            if (reusableIR != nullptr) {
                auto removeDef = [](IRCode* &reusableIR, const string& dst) {
                    if (reusableIR == nullptr) {
                        return;
                    }
                    switch (reusableIR->op) {
                    case IROperator::ADD:
                    case IROperator::SUB:
                    case IROperator::MUL:
                    case IROperator::DIV:
                        if (reusableIR->op1 == dst || reusableIR->op2 == dst) {
                            reusableIR = nullptr;
                        }
                        break;
                    case IROperator::MOV:
                        if (reusableIR->op1 == dst) {
                            reusableIR = nullptr;
                        }
                        break;
                    default:
                        break;
                    }
                };
                for (int k = j + 1; k < i; k++) {
                    if (reusableIR == nullptr)
                        break;
                    switch (block[k].op) {
                    case IROperator::ADD:
                    case IROperator::SUB:
                    case IROperator::MUL:
                    case IROperator::DIV:
                    case IROperator::LEQ:
                    case IROperator::LT:
                    case IROperator::GEQ:
                    case IROperator::GT:
                    case IROperator::NEQ:
                    case IROperator::EQU:
                        if (reusableIR->dst == block[k].dst) {
                            reusableIR = nullptr;
                        }
                        removeDef(reusableIR, block[k].dst);
                        break;
                    case IROperator::READ:
                        if (reusableIR->dst == block[k].op1) {
                            reusableIR = nullptr;
                        }
                        removeDef(reusableIR, block[k].op1);
                        break;
                    case IROperator::LOADARR:
                        if (reusableIR->dst == block[k].dst) {
                            reusableIR = nullptr;
                        }
                        removeDef(reusableIR, block[k].dst);
                        break;
                    case IROperator::SAVEARR:
                        if (reusableIR->dst == block[k].op1) {
                            reusableIR = nullptr;
                        }
                        removeDef(reusableIR, block[k].op1);
                        break;
                    case IROperator::MOV:
                        if (reusableIR->dst == block[k].dst) {
                            reusableIR = nullptr;
                        }
                        removeDef(reusableIR, block[k].dst);
                        break;
                    default:
                        break;
                    }
                }
            }

            if (reusableIR != nullptr) {
                if (cnt[code.dst] <= 1 && cnt[reusableIR->dst] <= 1) {
#ifdef DEBUG
                    printf("--DAG: reuse: [%d]%s -> [%d]%s\n", i,code.dumpString().c_str(), j,reusableIR->dumpString().c_str());
#endif
                }
                else {
#ifdef DEBUG
                    printf("--DAG: cross block: %s\n", reusableIR->dumpString().c_str());
                    printf("--DAG: cnt %s: %d\n", reusableIR->dst.c_str(), cnt[reusableIR->dst]);
#endif
                    reusableIR = nullptr;
                }
            }
            // Rewrite
            if (reusableIR != nullptr) {
                bool f = false;
                switch (code.op) {
                case IROperator::ADD:
                case IROperator::SUB:
                case IROperator::MUL:
                case IROperator::DIV:
                case IROperator::MOV:
                    f = true;
                    break;
                default:
                    break;
                }
                if (f)
                {
#ifdef DEBUG
                    printf("DAG:Skip IR %s\n", code.dumpString().c_str());
                    printf("%s %s\n", code.dst.c_str(), reusableIR->dst.c_str());
#endif
                    continue;
                }
                IRCode tmpCode(IROperator::MOV, reusableIR->dst, "", code.dst);
                tmpBlock.push_back(tmpCode);
                result.push_back(tmpCode);
            }
            else {
                tmpBlock.push_back(code);
                result.push_back(code);
            }
        }
        block = tmpBlock;
    }
    local.ircodes = result;
}

#endif

#ifdef OPT_MOVE

void optMove(FunctionBlock& local)
{
    const string& func = local.func;
    auto& ircodes = local.ircodes;
    auto& blocks = local.blocks;
    vector<IRCode> result;
    for (auto& block : blocks) {
        vector<IRCode> tmpBlock;
        int i = 0;
        for (i = 0; i < block.size(); ++i) {
            bool f = false;
            switch (block[i].op) {
            case IROperator::ADD:
            case IROperator::SUB:
            case IROperator::MUL:
            case IROperator::DIV:
                if (i + 1 < block.size() && block[i + 1].op == IROperator::MOV) {
                    if (block[i + 1].op1 == block[i].dst) {
#ifdef DEBUG
                        printf("STRIP %s\n", block[i + 1].dumpString().c_str());
#endif
                        block[i].dst = block[i + 1].dst;
                        f = true;
                    }
                }
                break;
            case IROperator::MOV:
                if (i + 1 < block.size() && block[i + 1].op == IROperator::MOV) {
                    if (block[i + 1].op1 == block[i].dst) {
#ifdef DEBUG
                        printf("STRIP %s\n", block[i + 1].dumpString().c_str());
#endif
                        block[i].dst = block[i + 1].dst;
                        f = true;
                    }
                }
                break;
            default:
                break;
            }
            tmpBlock.push_back(block[i]);
            result.push_back(block[i]);
            if (f) {
                ++i;
            }
        }
        block = tmpBlock;
    }
    local.ircodes = result;
}

#endif

void divideBlock(FunctionBlock& func) {
    const auto& ircodes = func.ircodes;
    vector<vector<IRCode>>& blocks = func.blocks;
    map<int, vector<int>>& nxt = func.blockNext;
    set<int> entry;
    entry.insert(0);
    entry.insert(ircodes.size());
    blocks.push_back(vector<IRCode>());
    for (int i = 0; i < ircodes.size(); i++) {
        switch (ircodes[i].op) {
        case IROperator::FUNC:
            entry.insert(i);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
        case IROperator::JUMP:
            entry.insert(i + 1);
            break;
        case IROperator::RET:
            entry.insert(i + 1);
            break;
        case IROperator::CALL:
            entry.insert(i + 1);
            break;
        case IROperator::LABEL:
            entry.insert(i);
            break;
        default:
            break;
        }
    }
    int cur = 0;
    int cnt = 0;
    for (auto it = entry.begin(); it != entry.end(); ++it) {
        for (; cur < *it; cur++) {
            if (cur >= ircodes.size()) {
                break;
            }
            blocks[cnt].push_back(ircodes[cur]);
        }
        blocks.push_back(vector<IRCode>());
        cnt++;
    }
    assert(blocks[0].size() == 0);
    assert(blocks[blocks.size() - 1].size() == 0);

    int Bexit = blocks.size() - 1;
    for (int i = 0; i < blocks.size() - 1; i++) {
        nxt[i] = vector<int>();
        int n = blocks[i].size();
        if (n == 0) {
            continue;
        }
        switch (blocks[i][n - 1].op) {
        case IROperator::BZ:
        case IROperator::BNZ: {
            string label = blocks[i][n - 1].op2;
            nxt[i].push_back(i + 1);
            for (int j = 0; j < blocks.size(); j++) {
                if (blocks[j].size() > 0 &&
                    blocks[j][0].op == IROperator::LABEL &&
                    blocks[j][0].op1 == label) {
                    nxt[i].push_back(j);
                    break;
                }
            }
            break;
        }
        case IROperator::JUMP: {
            string label = blocks[i][n - 1].op1;
            for (int j = 0; j < blocks.size(); j++) {
                if (blocks[j].size() > 0 &&
                    blocks[j][0].op == IROperator::LABEL &&
                    blocks[j][0].op1 == label) {
                    nxt[i].push_back(j);
                    break;
                }
            }
            break;
        }
        case IROperator::RET:
            nxt[i].push_back(Bexit);
            break;
        default:
            nxt[i].push_back(i + 1);
            break;
        }
    }
    // dump
    // for (auto it = entry.begin(); it != entry.end(); ++it) {
    //     cout << "//Block entry " << *it << endl;
    // }
    // for (const auto &p : nxt) {
    //     int s = p.first;
    //     for (int t : p.second) {
    //         cout << "//B" << s << "->B" << t << endl;
    //     }
    // }

    // for (int i = 0; i < blocks.size(); i++) {
    //     cout << "//Block " << i << endl;
    //     for (const auto &code : blocks[i]) {
    //         cout << code.dumpString() << endl;
    //     }
    // }
}

void divideFunction(const vector<IRCode>& ircodes,
    vector<FunctionBlock>& result) {
    map<string, int> strid;
    string curFunc = "";
    strid[curFunc] = result.size();
    result.push_back(FunctionBlock());
    for (int i = 0; i < ircodes.size(); i++) {
        switch (ircodes[i].op) {
        case IROperator::FUNC:
            curFunc = ircodes[i].op1;
            strid[curFunc] = result.size();
            result.push_back(FunctionBlock());
            result[strid[curFunc]].ircodes.push_back(ircodes[i]);
            result[strid[curFunc]].func = curFunc;
            break;
        default:
            result[strid[curFunc]].ircodes.push_back(ircodes[i]);
            break;
        }
    }
    assert(result.size() > 0);
}

#ifdef OPT_INLINE_IR

void optInlineIR(vector<IRCode>& ircodes)
{
    auto isConst = [](const string& var)->bool {
        int i = 0;
        if (var[0] == '-' || var[0] == '+') {
            i = 1;
        }
        else {
            i = 0;
        }
        for (; i < var.length(); i++) {
            if (!isdigit(var[i])) {
                return false;
            }
        }
        return true;
    };
    string curFunc = "";
    map<string, bool> canInline;
    map<string, set<string>> varList;
    vector<vector<IRCode>> funcCode;
    map<string, int> funcCodeIdx;
    funcCodeIdx[""] = funcCode.size();
    funcCode.push_back(vector<IRCode>());
    for (int i = 0; i < ircodes.size();i++) {
        if (ircodes[i].op == IROperator::FUNC) {
            curFunc = ircodes[i].op1;
            canInline[curFunc] = true;

            funcCodeIdx[curFunc] = funcCode.size();
            funcCode.push_back(vector<IRCode>());

            funcCode[funcCodeIdx[curFunc]].push_back(ircodes[i]);
        }
        else {
            funcCode[funcCodeIdx[curFunc]].push_back(ircodes[i]);
        }
    }

    for (auto& p : funcCodeIdx) {
        curFunc = p.first;
        for (const auto& code : funcCode[p.second])
        {
            switch (code.op) {
            case IROperator::ADD:
            case IROperator::SUB:
            case IROperator::MUL:
            case IROperator::DIV:
            case IROperator::LEQ:
            case IROperator::LT:
            case IROperator::GEQ:
            case IROperator::GT:
            case IROperator::NEQ:
            case IROperator::EQU:
                varList[curFunc].insert(code.dst);
                varList[curFunc].insert(code.op1);
                varList[curFunc].insert(code.op2);
                break;
            case IROperator::READ:
                varList[curFunc].insert(code.op1);
                break;
            case IROperator::WRITE:
                varList[curFunc].insert(code.op1);
                break;
            case IROperator::PUSH:
                varList[curFunc].insert(code.op1);
                break;
            case IROperator::RET:
                varList[curFunc].insert(code.op1);
                break;
            case IROperator::LOADARR:
                varList[curFunc].insert(code.dst);
                varList[curFunc].insert(code.op1);
                varList[curFunc].insert(code.op2);
                break;
            case IROperator::SAVEARR:
                varList[curFunc].insert(code.op1);
                varList[curFunc].insert(code.dst);
                varList[curFunc].insert(code.op2);
                break;
            case IROperator::BZ:
            case IROperator::BNZ:
                varList[curFunc].insert(code.op1);
                break;
            case IROperator::MOV:
                varList[curFunc].insert(code.dst);
                varList[curFunc].insert(code.op1);
                break;
            default:
                break;
            }
        }
    }

    int retCount = 0;
    int retPos = -1;
    curFunc = "";
    for (int i = 0; i < ircodes.size(); i++) {
        const auto& code = ircodes[i];
        switch (code.op) {
        case IROperator::FUNC:
            curFunc = code.op1;
            if (curFunc == "main") {
                canInline[curFunc] = false;
            }
            retCount = 0;
            retPos = i;
            while (retPos < ircodes.size()) {
                if (ircodes[retPos].op == IROperator::FUNC && ircodes[retPos].op1 != ircodes[i].op1) {
                    break;
                }
                ++retPos;
            }
            --retPos;
            break;
        case IROperator::CALL:
        case IROperator::PUSH:
        case IROperator::VAR:
            canInline[curFunc] = false;
            break;
        case IROperator::JUMP:
        case IROperator::BNZ:
        case IROperator::BZ:
            canInline[curFunc] = false;
            break;
        case IROperator::RET:
            ++retCount;
            if (retPos != i) {
#ifdef DEBUG
                printf("----no inline: Not return at the end\n");
#endif
                canInline[curFunc] = false;
            }
        default:
            break;
        }
    }
    for (auto& p : varList) {
        const string& func = p.first;
        if (func == "" || func == "main") {
            canInline[func] = false;
            continue;
        }

        for (const string& name : p.second) {
            if (name.length() > 0 && name[0] == '$') {
                continue;
            }
            if (isConst(name)) {
                continue;
            }
            if (name.length() > 1 && name[0] == '#' && name[0] == 't') {
                canInline[func] = false;
                break;
            }
            assert(sym_table.functionLocalSymbols.count(func));
            auto& localSym = sym_table.functionLocalSymbols[func];
            if (localSym.count(name) && localSym[name].clazz == Symbol::SYM_PARAM) {
                continue;
            }
            else {
                canInline[func] = false;
                break;
            }
        }
        if (sym_table.functionParams[func].size() > 3) {
            canInline[func] = false;
        }
    }


#ifdef DEBUG
    printf("----Inline IR\n");
    for (auto& p : canInline) {
        printf("%s: %d\n", p.first.c_str(), p.second);
    }
#endif
    vector<IRCode> result;

    for (auto& ircodes : funcCode) {
        string func = "";
        for (int i = 0; i < ircodes.size(); i++) {
            if (ircodes[i].op == IROperator::FUNC) {
                func = ircodes[i].op1;
                break;
            }
        }


        if (canInline.count(func) && canInline[func])
        {
#ifdef DEBUG
            printf("SKIP function define: %s\n", func.c_str());
#endif
            continue;
        }
        map<string, string> paramMap;
        for (int i = 0; i < ircodes.size();i++) {
            switch (ircodes[i].op) {
            case IROperator::PUSH:
                if (canInline.count(ircodes[i].dst) && canInline[ircodes[i].dst]) {
                    const auto& to = ircodes[i].dst;
                    int idx = stoi(ircodes[i].op2);
                    assert(sym_table.functionParams.count(to));
                    const auto& para = sym_table.functionParams[to][idx].name;
                    paramMap[para] = "$a" + to_string(stoi(ircodes[i].op2) + 1);
                    result.push_back(IRCode(IROperator::MOV, ircodes[i].op1, "", paramMap[para]));
                }
                else {
                    result.push_back(ircodes[i]);
                }
                break;
            case IROperator::CALL:
                if (canInline.count(ircodes[i].op1) && canInline[ircodes[i].op1]) {
                    const auto& to = ircodes[i].op1;
#ifdef DEBUG
                    printf("--PARAM MAPPING(%s)\n", to.c_str());
                    for (auto& pp : paramMap) {
                        printf("%s -> %s\n", pp.first.c_str(), pp.second.c_str());
                    }
#endif
                    //BEGIN REWRITE
                    auto rewrite = [&](string& name) {
                        if (paramMap.count(name))
                        {
                            name = paramMap[name];
                        }
                    };
                    for (auto& targetCode : funcCode[funcCodeIdx[to]]) {
                        switch (targetCode.op) {
                        case IROperator::ADD:
                        case IROperator::SUB:
                        case IROperator::MUL:
                        case IROperator::DIV:
                        case IROperator::LEQ:
                        case IROperator::LT:
                        case IROperator::GEQ:
                        case IROperator::GT:
                        case IROperator::NEQ:
                        case IROperator::EQU:
                            rewrite(targetCode.op1);
                            rewrite(targetCode.op2);
                            rewrite(targetCode.dst);
                            result.push_back(targetCode);
                            break;
                        case IROperator::READ:
                            rewrite(targetCode.op1);
                            result.push_back(targetCode);
                            break;
                        case IROperator::WRITE:
                            rewrite(targetCode.op1);
                            result.push_back(targetCode);
                            break;
                        case IROperator::RET:
                            if (paramMap.count(targetCode.op1)) {
                                result.push_back(IRCode(IROperator::MOV, paramMap[targetCode.op1], "", "#RET"));
                            }
                            else {
                                result.push_back(IRCode(IROperator::MOV, targetCode.op1, "", "#RET"));
                            }
                            break;
                        case IROperator::MOV:
                            rewrite(targetCode.op1);
                            rewrite(targetCode.dst);
                            result.push_back(targetCode);
                            break;
                        case IROperator::CONSTANT:
                        case IROperator::FUNC:
                        case IROperator::PARA:
                            //SKIP
                            break;
                        case IROperator::VAR:
                        case IROperator::LOADARR:
                        case IROperator::SAVEARR:
                        case IROperator::CALL:
                        case IROperator::JUMP:
                        case IROperator::PUSH:
                        case IROperator::LABEL:
                        case IROperator::BZ:
                        case IROperator::BNZ:
                        default:
                            fprintf(stderr, "Unexpected code in inlined function\n");
                            //assert(0);
                            break;
                        }
                    }
                    //END REWRITE
                }
                else {
                    result.push_back(ircodes[i]);
                }
                paramMap.clear();
                break;
            default:
                result.push_back(ircodes[i]);
                break;
            }
        }
    }
    ircodes = result;

}

#endif

#ifdef OPT_REG_ALLOC

void graphRegisterAllocate(FunctionBlock& local) {
    auto& func = local.func;
    auto& blocks = local.blocks;
    auto& inSet = local.liveInSet;
    auto& outSet = local.liveOutSet;
    map<string, int> localVarId;
    vector<string> localVar;
    int** mat;

    if (func == "") {
        return;
    }

    for (const auto& p : sym_table.functionLocalSymbols[func]) {
        if (p.second.clazz == Symbol::SYM_VAR)
        {
            localVarId[p.first] = localVar.size();
            localVar.push_back(p.first);
        }
    }
    //allocate matrix
    mat = new int* [localVar.size()];
    for (int i = 0; i < localVar.size(); i++) {
        mat[i] = new int[localVar.size()];
    }

    for (int i = 0; i < localVar.size(); i++) {
        for (int j = 0; j < localVar.size(); j++) {
            mat[i][j] = 0;
        }
    }

    for (int k = 0; k < blocks.size(); k++) {
        for (const auto& i : localVar) {
            for (const auto& j : localVar) {
                if ((inSet[k].count(i) || outSet[k].count(i))
                    && (inSet[k].count(j) || outSet[k].count(j))) {
                    mat[localVarId[i]][localVarId[j]] = 1;
                }
            }
        }
    }

#ifdef DEBUG
    printf("----interference Graph\n");
    printf("%6s", "");
    for (int i = 0; i < localVar.size(); i++) {
        printf("%6s", localVar[i].c_str());
    }
    printf("\n");
    for (int i = 0; i < localVar.size(); i++) {
        printf("%6s", localVar[i].c_str());
        for (int j = 0; j < localVar.size(); j++) {
            printf("%6c", (mat[i][j] ? 'x' : ' '));
        }
        printf("\n");
    }
    printf("----graphviz\n");
    printf("graph g{\n");
    for (int i = 0; i < localVar.size(); i++) {
        for (int j = i + 1; j < localVar.size(); j++) {
            if (mat[i][j]) {
                printf("%s--%s;\n", localVar[i].c_str(), localVar[j].c_str());
            }
        }
    }
    printf("}\n");
#endif

    vector<string> globalReg = { "$s0", "$s1", "$s2", "$s3",
                             "$s4", "$s5", "$s6", "$s7" };
    map<string, string> regMap;

    auto coloring = [&]() {
        int n = localVar.size();
        vector<set<int>> graph;
        set<int> nodes;
        stack<int> s;
        map<int, int> color;
        for (int i = 0; i < n; i++) {
            graph.push_back(set<int>());
        }
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j && mat[i][j]) {
                    graph[i].insert(j);
                    nodes.insert(i);
                    nodes.insert(j);
                }
            }
        }
        while (1) {
            while (1) {
                int i;
                bool removed = false;
                for (i = 0; i < n; i++) {
                    if (!graph[i].empty() && graph[i].size() < globalReg.size())
                    {
                        break;
                    }
                }
                if (i < n) {
                    // Remove i from graph
                    if (!graph[i].empty())
                    {
                        s.push(i);
                        removed = true;
                        nodes.erase(i);
                        graph[i].clear();
                        for (int j = 0; j < n; j++)
                        {
                            if (graph[j].count(i))
                            {
                                graph[j].erase(i);
                            }
                        }
                    }
                }
                if (!removed) {
                    break;
                }
            }
            for (int i = 0; i < n; i++) {
                if (!graph[i].empty())
                {
#ifdef DEBUG
                    printf("GRAPH: NO REG\n");
                    printf("%s ", localVar[i].c_str());
                    printf("\n");
#endif
                    nodes.erase(i);
                    graph[i].clear();
                    for (int j = 0; j < n; j++)
                    {
                        if (graph[j].count(i))
                        {
                            graph[j].erase(i);
                        }
                    }
                    break;
                }
            }
            bool empty = true;
            for (int i = 0; i < n; i++) {
                if (!graph[i].empty()) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                break;
            }
        }
        for (auto& u : nodes) {
            s.push(u);
        }
#ifdef DEBUG
        printf("POP GRAPH\n");
#endif
        nodes.clear();
        for (int i = 0; i < n; i++)
        {
            graph[i].clear();
        }
        while (!s.empty())
        {
            int u = s.top();
            s.pop();

            for (int i = 0; i < n; i++)
            {
                if (mat[u][i])
                {
                    graph[u].insert(i);
                }
                if (mat[i][u])
                {
                    graph[i].insert(u);
                }
            }

            int c = 0;
            for (c = 0; c < globalReg.size(); c++)
            {
                bool used = false;
                for (int v : graph[u])
                {
                    if (color.count(v) && color[v] == c) {
                        used = true;
                    }
                }
                if (!used) {
                    color[u] = c;
                    break;
                }
            }


#ifdef DEBUG
            printf("%s ", localVar[u].c_str());
#endif
        }
#ifdef DEBUG
        printf("\n");
#endif
        for (auto& p : color) {
            regMap[localVar[p.first]] = globalReg[p.second];
        }
#ifdef DEBUG
        printf("COLOR:\n");
        for (auto& p : color) {
            printf("%s: %d\n", localVar[p.first].c_str(), p.second);
        }
#endif

    };

    auto assignReg = [&](string& name) {
        if (regMap.count(name) && regMap[name] != "") {
            name = regMap[name];
        }
    };

    coloring();
    set<string> remainReg = { "$s0", "$s1", "$s2", "$s3",
                             "$s4", "$s5", "$s6", "$s7" };
    for (const auto& p : regMap) {
        if (remainReg.count(p.second)) {
            remainReg.erase(p.second);
        }
    }
    for (const auto& v : localVar) {
        if (!regMap.count(v))
        {
            if (!remainReg.empty()) {
                regMap[v] = *(remainReg.begin());
                remainReg.erase(remainReg.begin());
            }
        }
    }

    // ASSIGN
    vector<IRCode> result;
    for (auto& block : blocks) {
        for (auto& ircode : block) {
            switch (ircode.op) {
            case IROperator::ADD:
            case IROperator::SUB:
            case IROperator::MUL:
            case IROperator::DIV:
                assignReg(ircode.op1);
                assignReg(ircode.op2);
                assignReg(ircode.dst);
                result.push_back(ircode);
                break;
            case IROperator::LEQ:
            case IROperator::LT:
            case IROperator::GEQ:
            case IROperator::GT:
            case IROperator::NEQ:
            case IROperator::EQU:
                assignReg(ircode.op1);
                assignReg(ircode.op2);
                //assignReg(ircode.dst);
                result.push_back(ircode);
                break;
            case IROperator::READ:
            case IROperator::WRITE:
                assignReg(ircode.op1);
                result.push_back(ircode);
                break;
            case IROperator::PUSH:
                assignReg(ircode.op1);
                result.push_back(ircode);
                break;
            case IROperator::RET:
                assignReg(ircode.op1);
                result.push_back(ircode);
                break;
            case IROperator::LOADARR:
                assignReg(ircode.op2);
                assignReg(ircode.dst);
                result.push_back(ircode);
                break;
            case IROperator::SAVEARR:
                assignReg(ircode.op2);
                assignReg(ircode.dst);
                result.push_back(ircode);
                break;
            case IROperator::BZ:
            case IROperator::BNZ:
                assignReg(ircode.op1);
                result.push_back(ircode);
                break;
            case IROperator::MOV:
                assignReg(ircode.op1);
                assignReg(ircode.dst);
                result.push_back(ircode);
                break;
            default:
                result.push_back(ircode);
                break;
            }
        }
    }

    local.ircodes = result;
#ifdef DEBUG
    printf("--GRAPH REG ALLOCATE(%s):\n", local.func.c_str());
    for (const auto& p : regMap) {
        printf("Assign: %s->%s\n", p.first.c_str(), p.second.c_str());
    }
#endif

    //clean up
    for (int i = 0; i < localVar.size(); i++) {
        delete[] mat[i];
    }
    delete[] mat;
}

void globalRegisterAllocate(FunctionBlock& local) {
    auto& ircodes = local.ircodes;
    const string& func = local.func;

    if (func == "") {
        return;
    }

    map<string, int> refCnt;
    //  "$a0" syscall
    // set<string> argReg = {"$a1", "$a2", "$a3"};
    set<string> globalReg = { "$s0", "$s1", "$s2", "$s3",
                             "$s4", "$s5", "$s6", "$s7" };
    map<string, string> regMap;

    auto updateRef = [&](const string& name) {
        assert(func != "");
        if (name[0] == '#') {
            return;
        }
        Symbol* s = sym_table.getByName(func, name);
        if (s != nullptr && !s->global &&
            (s->clazz == Symbol::SYM_PARAM || s->clazz == Symbol::SYM_VAR)) {
            if (!refCnt.count(s->name)) {
                refCnt[s->name] = 0;
            }
            refCnt[s->name]++;
        }
    };

    auto assignReg = [&](string& name) {
        if (regMap.count(name) && regMap[name] != "") {
            name = regMap[name];
        }
    };
    for (const auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            updateRef(ircode.op1);
            updateRef(ircode.op2);
            updateRef(ircode.dst);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            updateRef(ircode.op1);
            updateRef(ircode.op2);
            //updateRef(ircode.dst);
            break;
        case IROperator::READ:
            updateRef(ircode.op1);
            break;
        case IROperator::WRITE:
            updateRef(ircode.op1);
            break;
        case IROperator::PUSH:
            updateRef(ircode.op1);
            break;
        case IROperator::RET:
            if (ircode.op1 != "") {
                updateRef(ircode.op1);
            }
            break;
        case IROperator::LOADARR:
            updateRef(ircode.op2);
            updateRef(ircode.dst);
            break;
        case IROperator::SAVEARR:
            updateRef(ircode.op2);
            updateRef(ircode.dst);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            updateRef(ircode.op1);
            break;
        case IROperator::MOV:
            updateRef(ircode.op1);
            updateRef(ircode.dst);
            break;
        default:
            break;
        }
    }
    vector<string> tmp;
    for (const auto& p : refCnt) {
        tmp.push_back(p.first);
    }
    sort(tmp.begin(), tmp.end(), [&](const string& a, const string& b) {
        return refCnt[a] > refCnt[b];
        });

    for (const auto& p : tmp) {
        Symbol* s = sym_table.getByName(func, p);
        if (s->clazz == Symbol::SYM_VAR) {
            if (globalReg.empty()) {
                regMap[p] = "";
            }
            else {
                assert(s != nullptr);
                regMap[p] = *globalReg.begin();
                globalReg.erase(globalReg.begin());
            }
        }
    }
    vector<IRCode> result;
    for (auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            assignReg(ircode.op1);
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            assignReg(ircode.op1);
            assignReg(ircode.op2);
            //assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::READ:
        case IROperator::WRITE:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::PUSH:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::RET:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::LOADARR:
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::SAVEARR:
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::MOV:
            assignReg(ircode.op1);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        default:
            result.push_back(ircode);
            break;
        }
    }

    local.ircodes = result;
#ifdef DEBUG
    printf("--GRA result(%s):\n", local.func.c_str());
    for (const auto& p : regMap) {
        printf("Assign: %s->%s\n", p.first.c_str(), p.second.c_str());
    }
#endif
}

void tempRegisterAllocate(FunctionBlock& local) {
    auto& ircodes = local.ircodes;
    auto& blocks = local.blocks;
    //auto& blockNext = local.blockNext;
    auto& liveVarCount = local.liveVarCount;
    const string& func = local.func;
    vector<set<string>> useDefTmp;
    auto addTmp = [&](set<string>& s, const string& name) {
        if (name.length() > 2 && name[0] == '#' && name[1] == 't')
        {
            s.insert(name);
        }
        if (liveVarCount.count(name) && liveVarCount[name] == 0) {
            s.insert(name);
        }
    };

    if (func == "") {
        return;
    }

    for (int i = 0; i < blocks.size(); i++) {
        useDefTmp.push_back(set<string>());
        auto& tmpList = useDefTmp[i];
        for (const auto& ircode : blocks[i]) {
            switch (ircode.op) {
            case IROperator::ADD:
            case IROperator::SUB:
            case IROperator::MUL:
            case IROperator::DIV:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.op2);
                addTmp(tmpList, ircode.dst);
                break;
            case IROperator::LEQ:
            case IROperator::LT:
            case IROperator::GEQ:
            case IROperator::GT:
            case IROperator::NEQ:
            case IROperator::EQU:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.op2);
                //addTmp(tmpList,ircode.dst);
                break;
            case IROperator::READ:
            case IROperator::WRITE:
                addTmp(tmpList, ircode.op1);
                break;
            case IROperator::PUSH:
                addTmp(tmpList, ircode.op1);
                break;
            case IROperator::RET:
                if (ircode.op1 != "") {
                    addTmp(tmpList, ircode.op1);
                }
                break;
            case IROperator::LOADARR:
            case IROperator::SAVEARR:
                addTmp(tmpList, ircode.op2);
                addTmp(tmpList, ircode.dst);
                break;
            case IROperator::BZ:
            case IROperator::BNZ:
                //addTmp(tmpList,ircode.op1);
                break;
            case IROperator::MOV:
                addTmp(tmpList, ircode.op1);
                addTmp(tmpList, ircode.dst);
                break;
            default:
                break;
            }
        }
    }

    set<string> tmpList;
    map<string, string> regMap;
    // Not allocate reg for COMPARE & BRANCH
    for (const auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.op2);
            tmpList.insert(ircode.dst);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.op2);
            //tmpList.insert(ircode.dst);
            break;
        case IROperator::READ:
        case IROperator::WRITE:
            tmpList.insert(ircode.op1);
            break;
        case IROperator::PUSH:
            tmpList.insert(ircode.op1);
            break;
        case IROperator::RET:
            if (ircode.op1 != "") {
                tmpList.insert(ircode.op1);
            }
            break;
        case IROperator::LOADARR:
        case IROperator::SAVEARR:
            tmpList.insert(ircode.op2);
            tmpList.insert(ircode.dst);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            //tmpList.insert(ircode.op1);
            break;
        case IROperator::MOV:
            tmpList.insert(ircode.op1);
            tmpList.insert(ircode.dst);
            break;
        default:
            break;
        }
    }

    for (auto it = tmpList.begin(); it != tmpList.end();)
    {
        if ((*it).substr(0, 2) == "#t") {
            ++it;
        }
        else if (liveVarCount.count(*it) && liveVarCount[(*it)] == 0) {
            ++it;
        }
        else {
            it = tmpList.erase(it);
        }
    }

    map<string, int> cnt;
    for (const string& t : tmpList) {
        for (int i = 0; i < useDefTmp.size(); i++) {
            if (useDefTmp[i].count(t)) {
                cnt[t]++;
            }
        }
    }
    //  "$t8", "$t9" Reserved
    set<string> allTmpReg = { "$t0", "$t1", "$t2", "$t3",
                          "$t4", "$t5", "$t6", "$t7" };
    set<string> tmpReg = { "$t0", "$t1", "$t2", "$t3",
                          "$t4", "$t5", "$t6", "$t7" };
    for (const auto& blk : useDefTmp) {
        tmpReg = allTmpReg;
        for (const auto& t : blk) {
            if (cnt[t] > 1) {
                continue;
            }
            if (tmpReg.empty()) {
                regMap[t] = "";
            }
            else {
                regMap[t] = *tmpReg.begin();
                tmpReg.erase(tmpReg.begin());
            }
        }
    }
#ifdef DEBUG
    printf("--Temp result(%s):\n", local.func.c_str());
    printf("--TMPLIST:\n");
    for (auto it = tmpList.begin(); it != tmpList.end(); ++it)
    {
        printf("%s\n", it->c_str());
    }
    for (const auto& p : regMap) {
        printf("Assign: %s->%s\n", p.first.c_str(), p.second.c_str());
    }
    for (const auto& p : cnt) {
        if (p.second > 1) {
            printf("Cross-block: %s\n", p.first.c_str());
        }
    }
#endif
    auto assignReg = [&](string& name) {
        if (regMap.count(name) && regMap[name] != "") {
            name = regMap[name];
        }
    };

    vector<IRCode> result;
    for (auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            assignReg(ircode.op1);
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
            assignReg(ircode.op1);
            assignReg(ircode.op2);
            //assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::READ:
        case IROperator::WRITE:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::PUSH:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::RET:
            assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::LOADARR:
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::SAVEARR:
            assignReg(ircode.op2);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            //assignReg(ircode.op1);
            result.push_back(ircode);
            break;
        case IROperator::MOV:
            assignReg(ircode.op1);
            assignReg(ircode.dst);
            result.push_back(ircode);
            break;
        default:
            result.push_back(ircode);
            break;
        }
    }
}
#endif

void optimizeFunc(FunctionBlock& funcIR) {
    vector<IRCode> ircodes;
    divideBlock(funcIR);
    // for (const auto &block : funcIR.blocks) {
    //     vector<IRCode> tmp;
    //     constCombine(funcIR.func, block, tmp);
    // }
    funcIR.ircodes.clear();
    for (const auto& block : funcIR.blocks) {
        for (const auto& code : block) {
            funcIR.ircodes.push_back(code);
        }
    }
#ifdef OPT_CONST
    constSpread(funcIR);
#endif // OPT_CONST

#ifdef OPT_DAG
    optDag(funcIR);
#endif

#ifdef OPT_MOVE
    while (1) {
        int old = funcIR.ircodes.size();
        optMove(funcIR);
        if (old == funcIR.ircodes.size())
            break;
    }
#endif

#ifdef OPT_LIVE_DATAFLOW
    liveDataflow(funcIR);
#endif

#ifdef OPT_DCE
    DCE(funcIR);
#endif


}

void optimizeIR(const vector<IRCode>& ircodes, vector<IRCode>& optCode) {
#ifdef DEBUG
    FILE* f = fopen("quad.txt", "w");
    for (const auto& ir : ircodes) {
        fprintf(f, "%s\n", ir.dumpString().c_str());
    }
    fclose(f);
#endif
    vector<FunctionBlock> functions;
    divideFunction(ircodes, functions);
    for (auto& funcIR : functions) {
        optimizeFunc(funcIR);
#if defined(OPT_REG_ALLOC)
#ifdef OPT_GRAPH_COLOR
        graphRegisterAllocate(funcIR);
#else
        globalRegisterAllocate(funcIR);
#endif
        tempRegisterAllocate(funcIR);
#endif
        for (const auto& code : funcIR.ircodes) {
            optCode.push_back(code);
        }
    }
#ifdef OPT_INLINE_IR
    optInlineIR(optCode);
    functions.clear();
    divideFunction(optCode, functions);
    optCode.clear();
    for (auto& funcIR : functions) {
        optimizeFunc(funcIR);
        for (const auto& code : funcIR.ircodes) {
            optCode.push_back(code);
        }
    }
#endif
#ifdef DEBUG
    //printf("--optimize\n");
    f = fopen("opt_quad.txt", "w");
    for (const auto& code : optCode) {
        fprintf(f, "%s\n", code.dumpString().c_str());
    }
    fclose(f);
#endif
}
#endif
