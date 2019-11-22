#include "config.h"
// #if 1
#if !defined(USE_OPTIMIZE)
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include "IR.h"
#include "parser.h"
#include "symtab.h"
using std::string;
using std::to_string;
using std::stringstream;
using std::map;

unsigned int ROUND(unsigned int a, unsigned int n){
    return (((((unsigned int)(a))+(n)-1)) & ~((n)-1));
}
static string curFunc;
static FILE *fout;

void writeAsm(const std::string& mips)
{
    fprintf(fout, "%s\n", mips.c_str());
}

struct Frame {
    Symbol *f;
    unsigned int useRegs;
    map<string, int> tmpAddr;
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
        useRegs = (1<<31);
    }
    Frame(const string& funcName) {
        tmpSize = 0;
        f = sym_table.getByName("", funcName);
        useRegs = (1<<31);
    }
};
map<string,Frame> frames;

void getReg(const string &name, const string &rd) {
    // fprintf(stderr, "%s\n", name.c_str());
    stringstream ss;
    if (name == "#RET") {
        writeAsm("move " + rd + ", $v0");
        return;
    }
    if (name[0] == '#') {
        assert(frames.count(curFunc));
        if(!frames[curFunc].tmpAddr.count(name))
        {
            fprintf(stderr, "[DBG][Undefined]cur:%s name:%s\n", curFunc.c_str(),name.c_str());
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
    stringstream ss;
    if (name[0] == '#') {
        assert(frames.count(curFunc));
        int tmpOffset = frames[curFunc].allocTemp(name);
        // fprintf(stderr, "tmpOff:%d\n", tmpOffset);
        ss << "sw " << rd << ", " << tmpOffset << "($sp)";
        writeAsm(ss.str());
    } else {
        Symbol *s1 = sym_table.getByName(curFunc, name);
        if(s1 == nullptr){
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
            if (ircodes[i].dst[0] == '#' && ircodes[i].dst[1] == 't') {
                frames[func].allocTemp(ircodes[i].dst);
            }
        }
    }
}

void genFunc(const IRCode& ircode)
{
    const string& name = ircode.op1;
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
    fprintf(stderr,"[DBG]Frame %s size=%d\n",name.c_str(),framesize);
    Symbol *f = sym_table.getByName("", name);
    assert(frames.count(name));
    for(auto& p:frames[name].tmpAddr){
        p.second+=framesize;
    }
    framesize += frames[name].tmpSize;
    f->size = framesize;
    // if (!frames.count(name)) {
    //     frames[name] = Frame(name);
    // }
    curFunc = name;
    writeAsm("addiu $sp, $sp, " + to_string(-f->size));
    writeAsm("sw $ra, " + to_string(0)+"($sp)");
}

void genRet(const IRCode& ircode)
{
    if (curFunc == "main") {
        writeAsm("li $v0,10");
        writeAsm("syscall");
    } else {
        assert(frames.count(curFunc));
        Symbol *f = frames[curFunc].f;
        assert(f != nullptr);
        if(f->type==Symbol::SYM_INT||f->type==Symbol::SYM_CHAR)
        {
            getReg(ircode.op1,"$v0");
        }
        writeAsm("lw $ra, " + to_string(0) + "($sp)");
        writeAsm("addiu $sp, $sp, " + to_string(f->size));
        writeAsm("jr $ra");
    }
}

void genPush(const IRCode& ircode)
{
    const string &name = ircode.op1;
    int paramIdx = std::stoi(ircode.op2);
    const string &func = ircode.dst;

    string rt = "$t0";
    vector<Symbol> params = sym_table.getParams(func);
    Symbol *p = sym_table.getByName(func, params[paramIdx].name);
    assert(p);
    Symbol *f = sym_table.getByName("", func);
    assert(f);
    getReg(name, rt);
    writeAsm("sw " + rt + ", " + to_string(-f->size+p->addr) + "($sp)");
}

void genLabel(const IRCode& ircode)
{
    const string& name = ircode.op1;
    writeAsm(name + ":");
}

void genSyscall(int code)
{
    writeAsm("li $v0, "+std::to_string(code));
    writeAsm("syscall");
}

void genRead(const IRCode& ircode)
{
    string rs="$t0";
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

void genWrite(const IRCode& ircode)
{
    assert((ircode.op2 == "INT") || (ircode.op2 == "CHAR") ||
           (ircode.op2 == "STR"));
    if (ircode.op2 == "INT") {
        string rs="$a0";
        getReg(ircode.op1, rs);
        genSyscall(1);
    } else if (ircode.op2 == "CHAR") {
        string rs="$a0";
        getReg(ircode.op1, rs);
        genSyscall(11);
    } else if (ircode.op2 == "STR") {
        writeAsm("la $a0, " + ircode.op1);
        genSyscall(4);
    }
}

void genLoad(const IRCode& ircode)
{
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.op1);
    assert(base!=nullptr);
    string rs = "$t1", rt = "$t0";
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

void genStore(const IRCode& ircode)
{
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.op1);
    string rs = "$t1", rt = "$t0";
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
            ss << "addu " << rs << ", " << rs << ", " << "$sp";
            writeAsm(ss.str());
            ss.str("");
            ss << "sw " << rt << ", " << base->addr << "(" << rs << ")";
            writeAsm(ss.str());
        }
    }
}

void genArithmetic(const IRCode& ircode)
{
    stringstream ss;
    string rs = "$t1", rt = "$t2", rd = "$t0";
    getReg(ircode.op1, rs);
    getReg(ircode.op2, rt);
    switch (ircode.op) {
    case IROperator::ADD:
        ss << "addu " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::SUB:
        ss << "subu " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::MUL:
            ss << "mult " << rs << ", " << rt;
            writeAsm(ss.str());
            ss.str("");
            ss << "mflo " << rd;
        break;
    case IROperator::DIV:
            ss << "div " << rs << ", " << rt;
            writeAsm(ss.str());
            ss.str("");
            ss << "mflo " << rd;
        break;
    case IROperator::LEQ:
        ss << "sle " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::LT:
        ss << "slt " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::GEQ:
        ss << "sge " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::GT:
        ss << "sgt " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::NEQ:
        ss << "sne " << rd << ", " << rs << ", " << rt;
        break;
    case IROperator::EQU:
        ss << "seq " << rd << ", " << rs << ", " << rt;
        break;
    default:
        assert(0);
        break;
    }
    writeAsm(ss.str());
    saveReg(ircode.dst, rd);
}

void genBranch(const IRCode& ircode)
{
    string rs="$t0";
    getReg(ircode.op1, rs);
    if (ircode.op == IROperator::BZ) {
        writeAsm("beqz " + rs + ", " + ircode.op2);
    } else if (ircode.op == IROperator::BNZ) {
        writeAsm("bnez " + rs + ", " + ircode.op2);
    } else {
        assert(0);
    }
}

void genMove(const IRCode& ircode)
{
    stringstream ss;
    Symbol *base = sym_table.getByName(curFunc, ircode.dst);
    string rt = "$t0";
    getReg(ircode.op1, rt);
    saveReg(ircode.dst, rt);
}

void IR2mips(const IRCode& ircode)
{
    writeAsm("#"+ircode.dumpString());
    switch (ircode.op) {
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
        genArithmetic(ircode);
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

void genData()
{
    constexpr unsigned int DATA_BASE = 0x10010000;
    unsigned int size_cnt = 0;
    writeAsm(".data");
    for (auto& p : sym_table.globalSymbols) {
        Symbol& s = p.second;
        if (s.clazz == Symbol::SYM_ARRAY) {
            writeAsm(s.name + ":");
            if (s.type == Symbol::SYM_CHAR) {
                int align = ((s.size - 1) | 3) + 1;
                writeAsm(".space " + std::to_string(align));

                s.addr = DATA_BASE + size_cnt;
                size_cnt += align;
            }
            else if (s.type == Symbol::SYM_INT) {
                writeAsm(".space " + std::to_string(s.size * 4));

                s.addr = DATA_BASE + size_cnt;
                size_cnt += s.size * 4;
            }
            else {
                assert(0);
            }
        }
        else if (s.clazz == Symbol::SYM_VAR) {
            writeAsm(s.name + ":");
            if (s.type == Symbol::SYM_CHAR) {
                writeAsm(".word 0");

                s.addr = DATA_BASE + size_cnt;
                size_cnt += 4;
            }
            else if (s.type == Symbol::SYM_INT) {
                writeAsm(".word 0");

                s.addr = DATA_BASE + size_cnt;
                size_cnt += 4;
            }
            else {
                assert(0);
            }
        }
    }
    assert(size_cnt % 4 == 0);
    for (const auto& p : sym_table.stringId) {
        writeAsm(".string_" + std::to_string(p.second) + ":");
        string tmp;
        for (char c : p.first) {
            if (c == '\\') {
                tmp += "\\\\";
            }
            else {
                tmp += c;
            }
        }
        writeAsm(".asciiz \"" + tmp + "\"");
    }
    // writeAsm(".end: .word 0");
}

void objectCode(const std::vector<IRCode>& ircodes)
{
    //sym_table.dump();
    fout = fopen("mips.txt", "w");
    assert(fout);
    genData();
    scanTmp(ircodes);
    writeAsm(".text");
    writeAsm(".globl main");
    writeAsm("j main");

    for (const auto &code : ircodes) {
        IR2mips(code);
    }
    fclose(fout);
    //sym_table.dump();
}
#endif