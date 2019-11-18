#include "config.h"
// #if 0
#if defined(USE_OPTIMIZE)
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include "IR.h"
#include "parser.h"
#include "symtab.h"
using namespace std;

unsigned int ROUND(unsigned int a, unsigned int n) {
    return (((((unsigned int)(a)) + (n)-1)) & ~((n)-1));
}
static string curFunc;
static FILE *fout;

void writeAsm(const std::string &mips) { fprintf(fout, "%s\n", mips.c_str()); }

static constexpr int NO_REG = -1;
struct Frame {
    Symbol *f;
    map<string, int> tmpAddr;
    set<string> useGlobalReg;
    bool isLeaf;
    map<string, string> argReg;
    int tmpSize;
    int allocTemp(const string &name) {
        if (!tmpAddr.count(name)) {
            tmpAddr[name] = tmpSize;
            tmpSize += 4;
        }
        return tmpAddr[name];
    }

    Frame() {
        tmpSize = 0;
        f = nullptr;
        isLeaf = true;
    }
    Frame(const string &funcName) {
        tmpSize = 0;
        f = sym_table.getByName("", funcName);
        isLeaf = true;
    }
};
map<string, Frame> frames;

static bool isConst(const string &var) {
    int i = 0;
    if (var[0] == '-' || var[0] == '+') {
        i = 1;
    } else {
        i = 0;
    }
    for (; i < var.length(); i++) {
        if (!isdigit(var[i])) {
            return false;
        }
    }
    return true;
}

void getReg(const string &name, const string &rd) {
    // fprintf(stderr, "%s\n", name.c_str());
    assert(name.length() > 0);
    if (name[0] == '$') {
        writeAsm("move " + rd + ", " + name);
        return;
    }
#ifdef OPT_LEAF_FUNC
    assert(frames.count(curFunc));
    if (frames[curFunc].argReg.count(name)) {
        writeAsm("move " + rd + ", " + frames[curFunc].argReg[name]);
        return;
    }
#endif
    stringstream ss;
    if (name == "#RET") {
        writeAsm("move " + rd + ", $v0");
        return;
    }
    if (name[0] == '#') {
        assert(frames.count(curFunc));
        if (!frames[curFunc].tmpAddr.count(name)) {
            fprintf(stderr, "[DBG][Undefined]cur:%s name:%s\n", curFunc.c_str(),
                    name.c_str());
            assert(0);
        }
        int tmpOffset = frames[curFunc].tmpAddr[name];
        // fprintf(stderr, "tmpOff:%d\n", tmpOffset);
        ss << "lw " << rd << ", " << tmpOffset << "($sp)";
        writeAsm(ss.str());
    } else {
        Symbol *s1 = sym_table.getByName(curFunc, name);
        if (s1 == nullptr) {
            ss << "li " << rd << ", " << name;
            writeAsm(ss.str());
            return;
        }
        if (s1->clazz == Symbol::SYM_CONST) {
            ss << "li " << rd << ", " << s1->value;
        } else {
            if (s1->global) {
                if (s1->type == Symbol::SYM_CHAR) {
                    ss << "lbu " << rd << ", " << s1->name;
                } else {
                    ss << "lw " << rd << ", " << s1->name;
                }
            } else {
                if (s1->type == Symbol::SYM_CHAR) {
                    ss << "lbu " << rd << ", " << s1->addr << "($sp)";
                } else {
                    ss << "lw " << rd << ", " << s1->addr << "($sp)";
                }
            }
        }
        writeAsm(ss.str());
    }
}

void saveReg(const string &name, const string &rd) {
    // fprintf(stderr, "%s\n", name.c_str());
    if (name[0] == '$') {
        writeAsm("move " + name + ", " + rd);
        return;
    }
#ifdef OPT_LEAF_FUNC
    assert(frames.count(curFunc));
    if (frames[curFunc].argReg.count(name)) {
        writeAsm("move " + frames[curFunc].argReg[name] + ", " + rd);
        return;
    }
#endif
    stringstream ss;
    if (name[0] == '#') {
        assert(frames.count(curFunc));
        int tmpOffset = frames[curFunc].allocTemp(name);
        // fprintf(stderr, "tmpOff:%d\n", tmpOffset);
        ss << "sw " << rd << ", " << tmpOffset << "($sp)";
        writeAsm(ss.str());
    } else {
        Symbol *s1 = sym_table.getByName(curFunc, name);
        if (s1 == nullptr) {
            fprintf(stderr, "saveReg %s\n", name.c_str());
        }
        assert(s1 != nullptr);
        assert(s1->clazz != Symbol::SYM_CONST);
        assert((s1->type == Symbol::SYM_CHAR) || (s1->type == Symbol::SYM_INT));
        if (s1->global) {
            if (s1->type == Symbol::SYM_CHAR) {
                ss << "sb " << rd << ", " << s1->name;
            } else {
                ss << "sw " << rd << ", " << s1->name;
            }
        } else {
            if (s1->type == Symbol::SYM_CHAR) {
                ss << "sb " << rd << ", " << s1->addr << "($sp)";
            } else {
                ss << "sw " << rd << ", " << s1->addr << "($sp)";
            }
        }

        writeAsm(ss.str());
    }
}

void scanTmp(const std::vector<IRCode> &ircodes) {
    string func;
    for (int i = 0; i < ircodes.size(); ++i) {
        if (ircodes[i].op == IROperator::FUNC) {
            func = ircodes[i].op1;
            frames[func] = Frame(func);
        } else {
            assert(frames.count(func));
            if (ircodes[i].dst[0] == '#' && ircodes[i].dst[1] == 't') {
                frames[func].allocTemp(ircodes[i].dst);
            } else if (ircodes[i].dst[0] == '$' && ircodes[i].dst[1] == 's') {
                frames[func].allocTemp(ircodes[i].dst);
                frames[func].useGlobalReg.insert(ircodes[i].dst);
            }
        }
    }
#ifdef OPT_LEAF_FUNC
    func = "";
    for (const auto& ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::FUNC:
            func = ircode.op1;
            break;
        case IROperator::CALL:
        case IROperator::PUSH:
        case IROperator::VAR:
            frames[func].isLeaf = false;
            break;
        default:
            break;
        }
    }
#endif
#ifdef OPT_LEAF_FUNC
    if (sym_table.functionParams.count(func) && sym_table.functionParams[func].size()>3){
        frames[func].isLeaf = false;
    }
    if (frames[func].tmpAddr.size() > 0) {
        frames[func].isLeaf = false;
    }
#endif
#ifdef OPT_LEAF_FUNC
    // Rewrite
    
    func = "";
    for (const auto& ircode : ircodes) {
        
        if (ircode.op == IROperator::FUNC) {
            func = ircode.op1;
        }
        if (frames[func].isLeaf) {
            assert(sym_table.functionParams[func].size() <= 3);
            for (int i = 0; i < sym_table.functionParams[func].size(); i++) {
                const string& pname = sym_table.functionParams[func][i].name;
                frames[func].argReg[pname] = "$a" + to_string(i + 1);
            }
        }
    }
#endif
}

void genFunc(const IRCode &ircode) {
    const string &name = ircode.op1;
#ifdef OPT_LEAF_FUNC
    assert(frames.count(name));
    if (frames[name].isLeaf) {
        writeAsm("# LEAF FUNC");
        writeAsm(name + ":");
        //writeAsm("addiu $sp, $sp, -4");
        //writeAsm("sw $ra, " + to_string(0) + "($sp)");
        curFunc = name;
        return;
    }
#endif
    int size_cnt = 4;
    writeAsm("");
    writeAsm(name + ":");
    for (auto &p : sym_table.functionLocalSymbols[name]) {
        Symbol &s = p.second;
        if (s.clazz == Symbol::SYM_PARAM || s.clazz == Symbol::SYM_VAR) {
            s.addr = size_cnt;
            size_cnt += 4;
        } else if (s.clazz == Symbol::SYM_ARRAY) {
            if (s.type == Symbol::SYM_INT) {
                s.addr = size_cnt;
                size_cnt += 4 * s.size;
            } else if (s.type == Symbol::SYM_CHAR) {
                s.addr = size_cnt;
                size_cnt += ROUND(s.size, 4);
            } else {
                assert(0);
            }
        } else if (s.clazz == Symbol::SYM_CONST) {
            // Nothing
        } else {
            assert(0);
        }
    }
    int framesize = size_cnt;
    fprintf(stderr, "[DBG]Frame %s size=%d\n", name.c_str(), framesize);
    Symbol *f = sym_table.getByName("", name);
    assert(frames.count(name));
    for (auto &p : frames[name].tmpAddr) {
        p.second += framesize;
    }
    framesize += frames[name].tmpSize;
    f->size = framesize;
    // if (!frames.count(name)) {
    //     frames[name] = Frame(name);
    // }
    fprintf(stderr, "[DBG]Frame %s tmpsize=%d\n", name.c_str(),
            frames[name].tmpSize);
    curFunc = name;
    writeAsm("addiu $sp, $sp, " + to_string(-f->size));
    writeAsm("sw $ra, " + to_string(0) + "($sp)");
    for (const auto &r : frames[name].useGlobalReg) {
        writeAsm("sw " + r + ", " + to_string(frames[curFunc].tmpAddr[r]) +
                     "($sp)");
    }
}

void genRet(const IRCode &ircode) {
    if (curFunc == "main") {
        writeAsm("li $v0,10");
        writeAsm("syscall");
    } else {
        assert(frames.count(curFunc));
        Symbol *f = frames[curFunc].f;
        assert(f != nullptr);
        if (f->type == Symbol::SYM_INT || f->type == Symbol::SYM_CHAR) {
            string rd = "$v0";
            getReg(ircode.op1, rd);
        }
#ifdef OPT_LEAF_FUNC
        if (frames[curFunc].isLeaf) {
            writeAsm("# LEAF FUNC END");
            //writeAsm("lw $ra, " + to_string(0) + "($sp)");
            //writeAsm("addiu $sp, $sp, 4");
            writeAsm("jr $ra");
            return;
        }
#endif
        writeAsm("lw $ra, " + to_string(0) + "($sp)");
        for (const auto &r : frames[curFunc].useGlobalReg) {
            writeAsm("lw " + r + ", " + to_string(frames[curFunc].tmpAddr[r]) +
                     "($sp)");
        }
        writeAsm("addiu $sp, $sp, " + to_string(f->size));
        writeAsm("jr $ra");
    }
}

void genPush(const IRCode &ircode) {
    const string &name = ircode.op1;
    int paramIdx = std::stoi(ircode.op2);
    const string &func = ircode.dst;
#ifdef OPT_LEAF_FUNC
    if (frames[func].isLeaf && paramIdx < 3) {
        string rt = "$a" + to_string(paramIdx + 1);
        getReg(name, rt);
        return;
    }
#endif
    string rt = "$t8";
    vector<Symbol> params = sym_table.getParams(func);
    Symbol *p = sym_table.getByName(func, params[paramIdx].name);
    assert(p);
    Symbol *f = sym_table.getByName("", func);
    assert(f);
    getReg(name, rt);
    writeAsm("sw " + rt + ", " + to_string(-f->size + p->addr) + "($sp)");
}

void genLabel(const IRCode &ircode) {
    const string &name = ircode.op1;
    writeAsm(name + ":");
}

void genSyscall(int code) {
    writeAsm("li $v0, " + std::to_string(code));
    writeAsm("syscall");
}

void genRead(const IRCode &ircode) {
    string rs = "$t8";
    assert((ircode.op2 == "INT") || (ircode.op2 == "CHAR"));
    if (ircode.op2 == "INT") {
        genSyscall(5);
        writeAsm("move " + rs + ", $v0");
    } else if (ircode.op2 == "CHAR") {
        genSyscall(12);
        writeAsm("move " + rs + ", $v0");
    }
    saveReg(ircode.op1, rs);
}

void genWrite(const IRCode &ircode) {
    assert((ircode.op2 == "INT") || (ircode.op2 == "CHAR") ||
           (ircode.op2 == "STR"));
    if (ircode.op2 == "INT") {
        string rs = "$a0";
        getReg(ircode.op1, rs);
        genSyscall(1);
    } else if (ircode.op2 == "CHAR") {
        string rs = "$a0";
        getReg(ircode.op1, rs);
        genSyscall(11);
    } else if (ircode.op2 == "STR") {
        writeAsm("la $a0, " + ircode.op1);
        genSyscall(4);
    }
}

void genLoad(const IRCode &ircode) {
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.op1);
    assert(base != nullptr);
    string rs = "$t8", rt = "$t9";
    getReg(ircode.op2, rs);
    if (base->global) {
        if (base->type == Symbol::SYM_CHAR) {
            ss << "lbu " << rt << ", " << base->name << "(" << rs << ")";
            writeAsm(ss.str());
        } else {
            ss << "sll " << rs << ", " << rs << ", " << 2;
            writeAsm(ss.str());
            ss.str("");
            ss << "lw " << rt << ", " << base->name << "(" << rs << ")";
            writeAsm(ss.str());
        }
    } else {
        if (base->type == Symbol::SYM_CHAR) {
            ss << "addu " << rs << ", " << rs << ", " << base->addr;
            writeAsm(ss.str());
            ss.str("");
            ss << "lbu " << rt << ", " << base->addr << "($sp)";
            writeAsm(ss.str());
        } else {
            ss << "sll " << rs << ", " << rs << ", " << 2;
            writeAsm(ss.str());
            ss.str("");
            ss << "addu " << rs << ", " << rs << ", " << base->addr;
            writeAsm(ss.str());
            ss.str("");
            ss << "lw " << rt << ", " << base->addr << "($sp)";
            writeAsm(ss.str());
        }
    }
    saveReg(ircode.dst, rt);
}

void genStore(const IRCode &ircode) {
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.op1);
    string rs = "$t8", rt = "$t9";
    getReg(ircode.op2, rs);
    getReg(ircode.dst, rt);
    if (base->global) {
        if (base->type == Symbol::SYM_CHAR) {
            ss << "sb " << rt << ", " << base->name << "(" << rs << ")";
            writeAsm(ss.str());
        } else {
            ss << "sll " << rs << ", " << rs << ", " << 2;
            writeAsm(ss.str());
            ss.str("");
            ss << "sw " << rt << ", " << base->name << "(" << rs << ")";
            writeAsm(ss.str());
        }
    } else {
        if (base->type == Symbol::SYM_CHAR) {
            ss << "addu " << rs << ", " << rs << ", "
               << "$sp";
            writeAsm(ss.str());
            ss.str("");
            ss << "sb " << rt << ", " << base->addr << "(" << rs << ")";
            writeAsm(ss.str());
        } else {
            ss << "sll " << rs << ", " << rs << ", " << 2;
            writeAsm(ss.str());
            ss.str("");
            ss << "addu " << rs << ", " << rs << ", "
               << "(" << rs << ")";
            writeAsm(ss.str());
            ss.str("");
            ss << "sw " << rt << ", " << base->addr << rs;
            writeAsm(ss.str());
        }
    }
}

static vector<string> powerOf2 = {
    "1",         "2",        "4",        "8",         "16",        "32",
    "64",        "128",      "256",      "512",       "1024",      "2048",
    "4096",      "8192",     "16384",    "32768",     "65536",     "131072",
    "262144",    "524288",   "1048576",  "2097152",   "4194304",   "8388608",
    "16777216",  "33554432", "67108864", "134217728", "268435456", "536870912",
    "1073741824"};

void genArithmetic(const IRCode &ircode) {
    static map<IROperator, string> instruct = {
        {IROperator::ADD, "addu"}, {IROperator::SUB, "subu"},
        {IROperator::LEQ, "sle"},  {IROperator::GEQ, "sge"},
        {IROperator::EQU, "seq"},  {IROperator::NEQ, "sne"},
        {IROperator::LT, "slt"},   {IROperator::GT, "sgt"}
    };
    stringstream ss;
    string rs = "$t8", rt = "$t9", rd = "$t8";
    bool c1 = isConst(ircode.op1), c2 = isConst(ircode.op2);
    switch (ircode.op) {
    case IROperator::MUL:
        if (c1 && c2) {
            ss << "li " << rd << ", " << stoi(ircode.op1) * stoi(ircode.op2);
        } else if (find(powerOf2.begin(), powerOf2.end(), ircode.op1) !=
                   powerOf2.end()) {
            getReg(ircode.op2, rt);
            int shift = find(powerOf2.begin(), powerOf2.end(), ircode.op1) -
                        powerOf2.begin();
            ss << "sll " << rd << ", " << rt << ", " << shift;
            writeAsm(ss.str());
        } else if (find(powerOf2.begin(), powerOf2.end(), ircode.op2) !=
                   powerOf2.end()) {
            getReg(ircode.op1, rs);
            int shift = find(powerOf2.begin(), powerOf2.end(), ircode.op2) -
                        powerOf2.begin();
            ss << "sll " << rd << ", " << rs << ", " << shift;
            writeAsm(ss.str());
        } else {
            getReg(ircode.op1, rs);
            getReg(ircode.op2, rt);
            ss << "mult " << rs << ", " << rt;
            writeAsm(ss.str());
            ss.str("");
            ss << "mflo " << rd;
            writeAsm(ss.str());
        }
        break;
    case IROperator::DIV:
        if (c1 && c2) {
            ss << "li " << rd << ", " << stoi(ircode.op1) / stoi(ircode.op2);
        } else if (find(powerOf2.begin(), powerOf2.end(), ircode.op2) !=
                   powerOf2.end()) {
            getReg(ircode.op1, rs);
            int shift = find(powerOf2.begin(), powerOf2.end(), ircode.op2) -
                        powerOf2.begin();
            ss << "sra " << rd << ", " << rs << ", " << shift;
            writeAsm(ss.str());
        } else {
            getReg(ircode.op1, rs);
            getReg(ircode.op2, rt);
            ss << "div " << rs << ", " << rt;
            writeAsm(ss.str());
            ss.str("");
            ss << "mflo " << rd;
            writeAsm(ss.str());
        }
        break;
    case IROperator::ADD:
        if (c1 && c2) {
            ss << "li " << rd << ", " << stoi(ircode.op1) + stoi(ircode.op2);
        } else if (!c1 && c2) {
            getReg(ircode.op1, rs);
            ss << "addiu " << rd << ", " << rs << ", " << stoi(ircode.op2);
        } else if (c1 && !c2) {
            getReg(ircode.op2, rt);
            ss << "addiu " << rd << ", " << rt << ", " << stoi(ircode.op1);
        } else {
            getReg(ircode.op1, rs);
            getReg(ircode.op2, rt);
            ss << "addu " << rd << ", " << rs << ", " << rt;
        }
        writeAsm(ss.str());
        break;
    case IROperator::LT:
        if (c1 && c2) {
            ss << "li " << rd << ", " << (stoi(ircode.op1) < stoi(ircode.op2));
        } else if (!c1 && c2) {
            getReg(ircode.op1, rs);
            ss << "slti " << rd << ", " << rs << ", " << stoi(ircode.op2);
        } else {
            getReg(ircode.op1, rs);
            getReg(ircode.op2, rt);
            ss << "slt " << rd << ", " << rs << ", " << rt;
        }
        writeAsm(ss.str());
        break;
    case IROperator::SUB:
    case IROperator::LEQ:
    case IROperator::GEQ:
    case IROperator::GT:
    case IROperator::NEQ:
    case IROperator::EQU:
        assert(instruct.count(ircode.op));
        getReg(ircode.op1, rs);
        getReg(ircode.op2, rt);
        ss << instruct[ircode.op] << " " << rd << ", " << rs << ", " << rt;
        writeAsm(ss.str());
        break;
    default:
        assert(0);
        break;
    }
    saveReg(ircode.dst, rd);
}

void genBranch(const IRCode &ircode) {
    string rs = "$t8";
    if (ircode.op == IROperator::BZ) {
        if (isConst(ircode.op1)) {
            if (stoi(ircode.op1) == 0) {
                writeAsm("jmp " + ircode.op2);
            } else {
                // SKIP
            }
        } else {
            getReg(ircode.op1, rs);
            writeAsm("beqz " + rs + ", " + ircode.op2);
        }
    } else if (ircode.op == IROperator::BNZ) {
        if (isConst(ircode.op1)) {
            if (stoi(ircode.op1) == 0) {
                // SKIP
            } else {
                writeAsm("jmp " + ircode.op2);
            }
        } else {
            getReg(ircode.op1, rs);
            writeAsm("bnez " + rs + ", " + ircode.op2);
        }
    } else {
        assert(0);
    }
}

void genMove(const IRCode &ircode) {
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.dst);
    if (isConst(ircode.op1) && ircode.dst[0] == '$') {
        writeAsm("li " + ircode.dst + "," + ircode.op1);
        return;
    }
    string rt = "$t8";
    getReg(ircode.op1, rt);
    saveReg(ircode.dst, rt);
}

void genData() {
    constexpr unsigned int DATA_BASE = 0x10010000;
    unsigned int size_cnt = 0;
    writeAsm(".data");
    for (auto &p : sym_table.globalSymbols) {
        Symbol &s = p.second;
        if (s.clazz == Symbol::SYM_ARRAY) {
            writeAsm(s.name + ":");
            if (s.type == Symbol::SYM_CHAR) {
                int align = ((s.size - 1) | 3) + 1;
                writeAsm(".space " + std::to_string(align));

                s.addr = DATA_BASE + size_cnt;
                size_cnt += align;
            } else if (s.type == Symbol::SYM_INT) {
                writeAsm(".space " + std::to_string(s.size * 4));

                s.addr = DATA_BASE + size_cnt;
                size_cnt += s.size * 4;
            } else {
                assert(0);
            }
        } else if (s.clazz == Symbol::SYM_VAR) {
            writeAsm(s.name + ":");
            if (s.type == Symbol::SYM_CHAR) {
                writeAsm(".word 0");

                s.addr = DATA_BASE + size_cnt;
                size_cnt += 4;
            } else if (s.type == Symbol::SYM_INT) {
                writeAsm(".word 0");

                s.addr = DATA_BASE + size_cnt;
                size_cnt += 4;
            } else {
                assert(0);
            }
        }
    }
    assert(size_cnt % 4 == 0);
    for (const auto &p : sym_table.stringId) {
        writeAsm(".string_" + std::to_string(p.second) + ":");
        writeAsm(".asciiz \"" + p.first + "\"");
    }
    // writeAsm(".end: .word 0");
}

void objectCode(const std::vector<IRCode> &ircodes) {
    sym_table.dump();
    fout = fopen("mips.txt", "w");
    assert(fout);
    genData();
    scanTmp(ircodes);
    writeAsm(".text");
    writeAsm(".globl main");

    for (int i = 0; i < ircodes.size();i++) {
        const auto& ircode = ircodes[i];
        writeAsm("#" + ircode.dumpString());
        switch (ircode.op) {
        case IROperator::ADD:
        case IROperator::SUB:
        case IROperator::MUL:
        case IROperator::DIV:
            genArithmetic(ircode);
            break;
        case IROperator::LEQ:
        case IROperator::LT:
        case IROperator::GEQ:
        case IROperator::GT:
        case IROperator::NEQ:
        case IROperator::EQU:
#if defined(OPT_COMP_BRANCH)
            if (i + 1 < ircodes.size()) {
                static map<IROperator, string> instruct = {
                    {IROperator::LEQ, "ble"},  {IROperator::GEQ, "bge"},
                    {IROperator::EQU, "beq"},  {IROperator::NEQ, "bne"},
                    {IROperator::LT, "blt"},   {IROperator::GT, "bgt"}
                };
                static map<IROperator, string> instructRev = {
                    {IROperator::LEQ, "bgt"},  {IROperator::GEQ, "blt"},
                    {IROperator::EQU, "bne"},  {IROperator::NEQ, "beq"},
                    {IROperator::LT, "bge"},   {IROperator::GT, "ble"}
                };
                string rs = "$t8";
                string rt = "$t9";
                if ((ircodes[i + 1].op == IROperator::BNZ) && (ircodes[i].dst == ircodes[i+1].op1)) {
                    getReg(ircode.op1, rs);
                    getReg(ircode.op2, rt);
                    writeAsm(instruct[ircode.op] + " " + rs + ", " + rt + ", " + ircodes[i + 1].op2);
                    ++i;
                }
                else if ((ircodes[i + 1].op == IROperator::BZ) && (ircodes[i].dst == ircodes[i+1].op1)) {
                    getReg(ircode.op1, rs);
                    getReg(ircode.op2, rt);
                    writeAsm(instructRev[ircode.op] + " " + rs + ", " + rt + ", " + ircodes[i + 1].op2);
                    ++i;
                }
                else {
                    genArithmetic(ircode);
                }
            }
            else {
                genArithmetic(ircode);
            }
#else
            genArithmetic(ircode);
#endif
            break;
        case IROperator::READ:
            genRead(ircode);
            break;
        case IROperator::WRITE:
            genWrite(ircode);
            break;
        case IROperator::CALL:
            writeAsm("jal " + ircode.op1);
            break;
        case IROperator::JUMP:
            writeAsm("j " + ircode.op1);
            break;
        case IROperator::PUSH:
            genPush(ircode);
            break;
        case IROperator::RET:
            genRet(ircode);
            break;
        case IROperator::LOADARR:
            genLoad(ircode);
            break;
        case IROperator::SAVEARR:
            genStore(ircode);
            break;
        case IROperator::FUNC:
            genFunc(ircode);
            break;
        case IROperator::LABEL:
            genLabel(ircode);
            break;
        case IROperator::BZ:
        case IROperator::BNZ:
            genBranch(ircode);
            break;
        case IROperator::MOV:
            genMove(ircode);
            break;
        default:
            break;
        }
    }
    fclose(fout);
    sym_table.dump();
}
#endif