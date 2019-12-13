#include "config.h"
#ifdef USE_OPTIMIZE
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <iterator>
#include <cctype>
#include "IR.h"
#include "parser.h"
#include "optimize.h"

using namespace std;

template <typename T> set<T> set_union(const set<T>& a, const set<T>& b) {
    set<T> c;
    set_union(a.begin(), a.end(), b.begin(), b.end(),
        insert_iterator<set<T>>(c, c.begin()));
    return c;
}

template <typename T> set<T> set_difference(const set<T>& a, const set<T>& b) {
    set<T> c;
    set_difference(a.begin(), a.end(), b.begin(), b.end(),
        insert_iterator<set<T>>(c, c.begin()));
    return c;
}


void liveDataflow(FunctionBlock& funcBlock) {
    auto& func = funcBlock.func;
    auto& blocks = funcBlock.blocks;
    auto& blockNext = funcBlock.blockNext;
    vector<set<string>> inSet;
    vector<set<string>> outSet;
    vector<set<string>> useSet;
    vector<set<string>> defSet;
    set<string> localVar;

    for (const auto& p : sym_table.functionLocalSymbols[func]) {
        localVar.insert(p.first);
    }

    for (auto& block : blocks) {
        set<string> use;
        set<string> def;
        auto insertUse = [&](const string& name) {
            if (localVar.count(name) && !def.count(name)) {
                use.insert(name);
            }
        };

        auto insertDef = [&](const string& name) {
            if (localVar.count(name) && !use.count(name)) {
                def.insert(name);
            }
        };
        for (const auto& code : block) {
            //get use
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
                insertUse(code.op1);
                insertUse(code.op2);
                break;
            case IROperator::WRITE:
                insertUse(code.op1);
                break;
            case IROperator::PUSH:
                insertUse(code.op1);
                break;
            case IROperator::RET:
                insertUse(code.op1);
                break;
            case IROperator::LOADARR:
                insertUse(code.op1);
                insertUse(code.op2);
                break;
            case IROperator::SAVEARR:
                insertUse(code.op1);
                insertUse(code.op2);
                break;
            case IROperator::BZ:
            case IROperator::BNZ:
                insertUse(code.op1);
                break;
            case IROperator::MOV:
                insertUse(code.op1);
                break;
            default:
                break;
            }
            //get def
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
                insertDef(code.dst);
                break;
            case IROperator::READ:
                insertDef(code.op1);
                break;
            case IROperator::LOADARR:
                insertDef(code.dst);
                break;
            case IROperator::SAVEARR:
                insertDef(code.dst);
                break;
            case IROperator::MOV:
                insertDef(code.dst);
                break;
            default:
                break;
            }
        }
        useSet.push_back(use);
        defSet.push_back(def);
    }
    bool hasChange = false;
    for (int i = 0; i < blocks.size(); ++i) {
        inSet.push_back(set<string>());
        outSet.push_back(set<string>());
    }
    do {
        hasChange = false;
        for (int u = blocks.size() - 1; u >= 0; --u) {
            int prevOut = outSet[u].size();
            int prevIn = inSet[u].size();
            for (int v : blockNext[u]) {
                outSet[u] = set_union(outSet[u], inSet[v]);
            }
            inSet[u] = set_union(useSet[u], set_difference(outSet[u], defSet[u]));
            if (prevOut != outSet[u].size()) {
                hasChange = true;
            }
            if (prevIn != inSet[u].size()) {
                hasChange = true;
            }
        }
    } while (hasChange);
#ifdef DEBUG
    printf("----live varible Dataflow(%s)\n", func.c_str());
    for (int i = 0; i < blocks.size(); ++i) {
        //printf("use[%d]=", i);
        //for (const auto &s : useSet[i]) {
        //    printf("%s ", s.c_str());
        //}
        //printf("\n");
        //printf("def[%d]=", i);
        //for (const auto &s : defSet[i]) {
        //    printf("%s ", s.c_str());
        //}
        //printf("\n");
        printf("in[%d]=", i);
        for (const auto& s : inSet[i]) {
            printf("%s ", s.c_str());
        }
        printf("\n");
        printf("out[%d]=", i);
        for (const auto& s : outSet[i]) {
            printf("%s ", s.c_str());
        }
        printf("\n");
    }
#endif
    map<string, int> cnt;
    for (auto& v : localVar)
    {
        cnt[v] = 0;
        for (int i = 0; i < blocks.size(); i++) {
            if (inSet[i].count(v) || outSet[i].count(v)) {
                cnt[v]++;
            }
        }
    }
    funcBlock.liveVarCount = cnt;
#ifdef DEBUG
    for (auto& i : localVar)
    {
        printf("%s: %d\n", i.c_str(), cnt[i]);
    }
#endif
}

#endif
