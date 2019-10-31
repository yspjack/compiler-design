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
}

// Add global symbol
// handles NAME_REDEFINITION
void SymTable::addGlobal(int clazz, int type, const string& name) {
    assert(clazz == Symbol::SYM_CONST || clazz == Symbol::SYM_ARRAY || clazz == Symbol::SYM_VAR || clazz == Symbol::SYM_FUNC);

    if (globalSymbols.count(name)) {
        handleError(NAME_REDEFINITION, linenumber);
        return;
    }
    globalSymbols[name] = Symbol(clazz, type, name);
    if (clazz == Symbol::SYM_FUNC) {
        functionLocalSymbols[name] = std::map<string, Symbol>();
        functionParams[name] = vector<Symbol>();
    }
}

// Add local symbol if func!=""
// handles NAME_REDEFINITION
void SymTable::addLocal(const string& func, int clazz, int type, const string& name) {
    //printf("%s,%s,%d,%d\n", func.c_str(), name.c_str(), clazz, type);
    if (func == "")
    {
        addGlobal(clazz, type, name);
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
            else {
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
