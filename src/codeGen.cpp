#include <vector>
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

void writeAsm(const std::string& mips)
{
    puts(mips.c_str());
}

void getReg(const string& name, string& rd)
{
    stringstream ss;
    Symbol* s1 = sym_table.getByName(context.curFunc, name);
    if (s1->type == Symbol::SYM_CONST) {
        ss << "li " << rd << ", " << s1->value;
    }
    else {
        if (s1->global) {
            if (s1->type == Symbol::SYM_CHAR) {
                ss << "lbu " << rd << ", " << s1->name;
            }
            else {
                ss << "lw " << rd << ", " << s1->name;
            }
        }
        else {
            if (s1->type == Symbol::SYM_CHAR) {
                ss << "lbu " << rd << ", " << "-" << s1->addr << "($sp)";
            }
            else {
                ss << "lw " << rd << ", " << "-" << s1->addr << "($sp)";
            }
        }
    }
    writeAsm(ss.str());
}

void genFunc(const IRCode& ircode)
{
    const string& name = ircode.op1;
    int size_cnt = 0;
    writeAsm(name + ":");
    for (auto& p : sym_table.functionLocalSymbols[name]) {
        Symbol& s = p.second;
        s.addr = size_cnt;
        size_cnt += 4;
    }
}

void genLabel(const IRCode& ircode)
{
    const string& name = ircode.op1;
    writeAsm(name + ":");
}

void genRead(const IRCode& ircode)
{

}

void genWrite(const IRCode& ircode)
{

}

void genLoad(const IRCode& ircode)
{

}

void genStore(const IRCode& ircode)
{

}

void genArithmetic(const IRCode& ircode)
{
    stringstream ss;
    string rs = "$t1", rt = "$t2";
    getReg(ircode.op1, rs);
    getReg(ircode.op2, rt);
    switch (ircode.op)
    {
    case IROperator::ADD:
        break;
    case IROperator::SUB:
        break;
    case IROperator::MUL:
        break;
    case IROperator::DIV:
        break;
    default:
        assert(0);
        break;
    }
}

void IR2mips(const IRCode& ircode)
{
    switch (ircode.op) {
    case IROperator::ADD:
    case IROperator::SUB:
    case IROperator::MUL:
    case IROperator::DIV:
        break;
    case IROperator::LEQ:
    case IROperator::LT:
    case IROperator::GEQ:
    case IROperator::GT:
    case IROperator::NEQ:
    case IROperator::EQU:
        break;
    case IROperator::READ:
        break;
    case IROperator::WRITE:
        break;
    case IROperator::CALL:
        break;
    case IROperator::JUMP:
        break;
    case IROperator::PUSH:
        break;
    case IROperator::RET:
        break;
    case IROperator::LOADARR:
        break;
    case IROperator::SAVEARR:
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
                int align = s.size;
                align = ((s.size - 1) | 3) + 1;
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
        writeAsm(".asciiz \"" + p.first + "\"");
    }
}

void objectCode(const std::vector<IRCode>& ircodes)
{
    genData();
    writeAsm(".text");
    writeAsm(".globl main");

    for (const auto& code : ircodes) {
        IR2mips(code);
    }
}