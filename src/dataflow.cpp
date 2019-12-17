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
        if (p.second.clazz == Symbol::SYM_VAR)
        {
            localVar.insert(p.first);
        }
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
                insertUse(code.dst);
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
                insertDef(code.op1);
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
    funcBlock.liveInSet = inSet;
    funcBlock.liveOutSet = outSet;
#ifdef DEBUG
    for (auto& i : localVar)
    {
        printf("%s: %d\n", i.c_str(), cnt[i]);
    }
#endif
}

void liveDataflowTmp(FunctionBlock& funcBlock, vector<set<string>>& inSet, vector<set<string>>& outSet) {
    auto& func = funcBlock.func;
    auto& blocks = funcBlock.blocks;
    auto& blockNext = funcBlock.blockNext;
    vector<set<string>> useSet;
    vector<set<string>> defSet;

    auto isVar = [&](const string& name)->bool {
        if (name.length() > 2) {
            if (name[0] == '#' && name[1] == 't') {
                return true;
            }
        }
        if (sym_table.functionLocalSymbols.count(func)) {
            if (sym_table.functionLocalSymbols[func].count(name)
                && sym_table.functionLocalSymbols[func][name].clazz == Symbol::SYM_VAR) {
                return true;
            }

        }
        return false;
    };

    for (auto& block : blocks) {
        set<string> use;
        set<string> def;
        auto insertUse = [&](const string& name) {
            if (isVar(name) && !def.count(name)) {
                use.insert(name);
            }
        };

        auto insertDef = [&](const string& name) {
            if (isVar(name) && !use.count(name)) {
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
                insertUse(code.dst);
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
                insertDef(code.op1);
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
    printf("----liveness with temp(%s)\n", func.c_str());
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
}


#ifdef OPT_DCE
void DCE_pass(FunctionBlock& local) {
    auto& blockNext = local.blockNext;
    auto& blocks = local.blocks;
    auto& func = local.func;
    auto& ircodes = local.ircodes;
    vector<set<string>> liveInSet;
    vector<set<string>> liveOutSet;
    auto isVar = [&](const string& name)->bool {
        if (name.length() > 2) {
            if (name[0] == '#' && name[1] == 't') {
                return true;
            }
        }
        if (sym_table.functionLocalSymbols.count(func)) {
            if (sym_table.functionLocalSymbols[func].count(name)
                && sym_table.functionLocalSymbols[func][name].clazz==Symbol::SYM_VAR) {
                return true;
            }

        }
        return false;
    };

    liveDataflowTmp(local, liveInSet, liveOutSet);

    vector<IRCode> result;
    for (int blkno = 0; blkno < blocks.size(); blkno++) {
        auto& blk = blocks[blkno];
        vector<set<string>> inSet;
        vector<set<string>> outSet;
        vector<set<string>> useSet;
        vector<set<string>> defSet;

        for (auto& code : blk) {
            set<string> use;
            set<string> def;
            auto insertUse = [&](const string& name) {
                if (isVar(name) && !def.count(name)) {
                    use.insert(name);
                }
            };

            auto insertDef = [&](const string& name) {
                if (isVar(name) && !use.count(name)) {
                    def.insert(name);
                }
            };
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
                insertUse(code.dst);
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
                insertDef(code.op1);
                break;
            case IROperator::MOV:
                insertDef(code.dst);
                break;
            default:
                break;
            }
            useSet.push_back(use);
            defSet.push_back(def);
        }
        for (int i = 0; i < blk.size(); ++i) {
            inSet.push_back(set<string>());
            outSet.push_back(set<string>());
        }
        if (blk.size() > 0) {
            inSet[0] = liveInSet[blkno];
            outSet[blk.size() - 1] = liveOutSet[blkno];
        }
        for (int u = blk.size() - 1; u >= 0; --u) {
            if (u + 1 < blk.size())
            {
                outSet[u] = set_union(outSet[u], inSet[u + 1]);
            }
            inSet[u] = set_union(useSet[u], set_difference(outSet[u], defSet[u]));
        }
        //if (blk.size() > 0) {
        //    inSet[0] = liveInSet[blkno];
        //    for (auto& u : inSet[0]) {
        //        printf("%s ", u.c_str());
        //    }
        //    printf("\n");
        //    for (auto& u : liveInSet[blkno]) {
        //        printf("%s ", u.c_str());
        //    }
        //    printf("\n");
        //}
        vector<IRCode> tmpBlock;

        for (int i = 0; i < blk.size(); ++i) {
            bool f = false;
            auto& code = blk[i];
            auto isDeadCode = [&](const string& name) {
                if (isVar(name) && !outSet[i].count(name))
                {
                    f = true;
                }
            };
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
                isDeadCode(code.dst);
                break;
            //case IROperator::READ:
            //    isDeadCode(code.op1);
            //    break;
            //case IROperator::LOADARR:
            //    isDeadCode(code.dst);
            //    break;
            //case IROperator::SAVEARR:
            //    isDeadCode(code.op1);
            //    break;
            case IROperator::MOV:
                isDeadCode(code.dst);
                break;
            default:
                break;
            }
            if (!f) {
                result.push_back(code);
                tmpBlock.push_back(code);
            }
#ifdef DEBUG
            if (f) {
                printf("----DCE: %s\n", code.dumpString().c_str());
            }
#endif
        }
        blk = tmpBlock;
    }
    local.ircodes = result;
}
void DCE(FunctionBlock& local)
{
    while (1)
    {
        int oldSize = local.ircodes.size();
        DCE_pass(local);
        if (oldSize == local.ircodes.size()) {
            break;
        }
    }
}
#endif

#endif
