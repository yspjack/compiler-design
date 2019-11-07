#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include "lexer.h"
#include "symtab.h"
#include "errproc.h"
using namespace std;

Symbol::Symbol() :clazz(-1), type(-1), name("") {}
Symbol::Symbol(int clazz, int type, const string& name) : clazz(clazz), type(type), name(name) {
    assert((clazz == SYM_FUNC) || (type != SYM_VOID));
    if (clazz != SYM_FUNC) {
        if (type == SYM_INT) {
            size = sizeof(int);
        }
        else if (type == SYM_CHAR) {
            size = sizeof(char);
        }
    }
    addr = 0;
    value = 0;
}

// Add global symbol
// handles NAME_REDEFINITION
void SymTable::addGlobal(int clazz, int type, const string& name, int value) {
    assert(clazz == Symbol::SYM_CONST || clazz == Symbol::SYM_ARRAY || clazz == Symbol::SYM_VAR || clazz == Symbol::SYM_FUNC);

    if (globalSymbols.count(name)) {
        handleError(NAME_REDEFINITION, linenumber);
        return;
    }
    Symbol s = Symbol(clazz, type, name);
    s.value = value;
    s.global = true;
    globalSymbols[name] = s;
    if (clazz == Symbol::SYM_FUNC) {
        functionLocalSymbols[name] = std::map<string, Symbol>();
        functionParams[name] = vector<Symbol>();
    }
}

// Add local symbol if func!=""
// handles NAME_REDEFINITION
void SymTable::addLocal(const string& func, int clazz, int type, const string& name, int value) {
    //printf("%s,%s,%d,%d\n", func.c_str(), name.c_str(), clazz, type);
    if (func == "")
    {
        addGlobal(clazz, type, name, value);
    }
    else {
        assert(functionLocalSymbols.count(func));
        assert(functionParams.count(func));
        assert(clazz == Symbol::SYM_CONST || clazz == Symbol::SYM_ARRAY || clazz == Symbol::SYM_VAR || clazz == Symbol::SYM_PARAM);
        if (functionLocalSymbols[func].count(name)) {
            handleError(NAME_REDEFINITION, linenumber);
            return;
        }
        Symbol s = Symbol(clazz, type, name);
        s.value = value;
        s.global = false;
        functionLocalSymbols[func][name] = s;
        if (clazz == Symbol::SYM_PARAM) {
            functionParams[func].push_back(s);
        }
    }
}

Symbol* SymTable::getByName(const string& func, const string& name) {
    if (func == "") {
        if (globalSymbols.count(name))
        {
            return &globalSymbols[name];
        }
    }
    else {
        if (functionLocalSymbols.count(func)) {
            if (functionLocalSymbols[func].count(name)) {
                return &functionLocalSymbols[func][name];
            }
            if (globalSymbols.count(name))
            {
                return &globalSymbols[name];
            }
        }
    }
    return nullptr;
}

vector<Symbol>& SymTable::getParams(const string& func) {
    assert(functionParams.count(func));
    return functionParams[func];
}

void SymTable::addString(const string& value) {
    if (stringId.count(value)) {
        return;
    }
    stringPool.push_back(value);
    stringId[value] = stringPool.size() - 1;
}

void dumpSymbol(const Symbol& s) {
    static map<int, string > typeStr = {
        {Symbol::SYM_ARRAY,"ARRAY"},
        {Symbol::SYM_CHAR,"CHAR"},
        {Symbol::SYM_CONST,"CONST"},
        {Symbol::SYM_FUNC,"FUNC"},
        {Symbol::SYM_INT,"INT"},
        {Symbol::SYM_PARAM,"PARAM"},
        {Symbol::SYM_VAR,"VAR"},
        {Symbol::SYM_VOID,"VOID"}
    };
    printf("%s %s %s", s.name.c_str(), typeStr[s.clazz].c_str(), typeStr[s.type].c_str());
    if (s.clazz == Symbol::SYM_CONST) {
        if (s.type == Symbol::SYM_CHAR) {
            printf(" value=\'%c\'", (char)s.value);
        }
        else {
            printf(" value=%d", s.value);
        }
    }
    else if (s.clazz == Symbol::SYM_ARRAY) {
        printf(" size=%d", s.size);
    }
    printf(" addr=%x", s.addr);
    printf("\n");
}
void SymTable::dump() {

    printf("----Global\n");
    for (const auto& p : globalSymbols) {
        const Symbol& s = p.second;
        dumpSymbol(s);
    }
    for (const auto& p1 : functionLocalSymbols) {
        printf("----Function %s\n", p1.first.c_str());
        for (const auto& p : functionLocalSymbols[p1.first]) {
            const Symbol& s = p.second;
            dumpSymbol(s);
        }
    }
}