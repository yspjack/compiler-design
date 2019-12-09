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
        case IROperator::READ:
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
        for (auto& code : blk) {
            bool c1 = false, c2 = false;
            int a, b;
            rewriteConst(code);
            if (code.dst.length() > 2 && code.dst.substr(0, 2) == "#t") {
                switch (code.op) {
                case IROperator::ADD:
                case IROperator::SUB:
                case IROperator::MUL:
                case IROperator::DIV:
                    c1 = getConst(func, code.op1, a);
                    c2 = getConst(func, code.op2, b);
                    if (c1 && c2) {
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
                    }
                    break;
                case IROperator::MOV:
                    c1 = getConst(func, code.op1, a);
                    if (c1) {
                        tmpConst[code.dst] = to_string(a);
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
            else {
                result.push_back(code);
            }
        }
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
    map<string, int> cnt = crossBlockTmp(local);
    vector<IRCode> result;
    map<string, string> tmpMap;
    auto findReuse = [](vector<IRCode>& block, IRCode& code)->IRCode* {
        for (IRCode& ircode : block) {
            if (ircode.op == code.op) {
                switch (ircode.op) {
                case IROperator::ADD:
                case IROperator::SUB:
                case IROperator::MUL:
                case IROperator::DIV:
                    if (ircode.op1 == code.op1 && ircode.op2 == code.op2) {
                        return &ircode;
                    }
                    break;
                case IROperator::MOV:
                    if (ircode.op1 == code.op1 && code.op1 != "#RET") {
                        return &ircode;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        return nullptr;
    };
    // Scan reusable code
    for (auto& p : cnt) {
        tmpMap[p.first] = p.first;
    }
    for (auto& block : local.blocks) {
        for (IRCode& code : block) {
            IRCode* reusableIR = findReuse(block, code);
            if (reusableIR != nullptr) {
                if (cnt[code.dst] == 1 && cnt[reusableIR->dst] == 1) {
                    tmpMap[code.dst] = reusableIR->dst;
                }
            }
        }
    }
    for (auto& block : local.blocks) {
        vector<IRCode> tmpBlock;
        for (IRCode& code : block) {

            if (tmpMap.count(code.dst) && tmpMap[code.dst] != code.dst) {
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
                    printf("DAG:Skip IR %s\n", code.dumpString().c_str());
                    printf("%s %s\n", code.dst.c_str(), tmpMap[code.dst].c_str());

                    continue;
                }
            }
            IRCode tmpCode = code;
            if (tmpMap.count(tmpCode.op1)) {
                tmpCode.op1 = tmpMap[tmpCode.op1];
            }
            if (tmpMap.count(tmpCode.op2)) {
                tmpCode.op2 = tmpMap[tmpCode.op2];
            }
            tmpBlock.push_back(tmpCode);
            result.push_back(tmpCode);
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
    // entry.insert(0);
    // entry.insert(ircodes.size());
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
    //  "$t8", "$t9" Reserved
    set<string> tmpReg = { "$t0", "$t1", "$t2", "$t3",
                          "$t4", "$t5", "$t6", "$t7" };
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
    auto& blockNext = local.blockNext;
    const string& func = local.func;
    vector<set<string>> useDefTmp;
    auto addTmp = [&](set<string>& s, const string& name) {
        if (name.length() > 2 && name[0] == '#' && name[1] == 't')
        {
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
            if (cnt[t] != 1) {
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
        if (p.second != 1) {
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

void optimizeFunc(FunctionBlock& funcIR, vector<IRCode>& optCode) {
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
#if 0
    liveDataflow(funcIR);
#endif
#ifdef OPT_CONST
    constSpread(funcIR);
#endif // OPT_CONST

#ifdef OPT_DAG
    optDag(funcIR);
#endif

#ifdef OPT_MOVE
    optMove(funcIR);
#endif


#if defined(OPT_REG_ALLOC)
    globalRegisterAllocate(funcIR);
    tempRegisterAllocate(funcIR);
#endif
    for (const auto& code : funcIR.ircodes) {
        optCode.push_back(code);
    }
}

void optimizeIR(const vector<IRCode>& ircodes, vector<IRCode>& optCode) {
#ifdef DEBUG
    FILE* f = fopen("quad.txt", "w");
    for (const auto& ir : ircodes) {
        fprintf(f, "%s\n", ir.dumpString().c_str());
    }
#endif
    vector<FunctionBlock> functions;
    divideFunction(ircodes, functions);
    for (auto& funcIR : functions) {
        optimizeFunc(funcIR, optCode);
    }
#ifdef DEBUG
    //printf("--optimize\n");
    f = fopen("opt_quad.txt", "w");
    for (const auto& code : optCode) {
        fprintf(f, "%s\n", code.dumpString().c_str());
    }
#endif
}
#endif
