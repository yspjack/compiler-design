#pragma once
#include <map>
#include <vector>
#include <string>
using std::map;
using std::vector;
using std::string;
struct Symbol {
    enum SymbolType {
        SYM_INT,
        SYM_CHAR,
        SYM_VOID,

        SYM_FUNC,
        SYM_PARAM,
        SYM_VAR,
        SYM_ARRAY,
        SYM_CONST
    };
    int clazz;
    int type;
    string name;
    int size;
    void* addr;
    Symbol();
    Symbol(int clazz, int type, const string& name);
};


struct SymTable
{
    map<string, Symbol > globalSymbols;
    map<string, map<string, Symbol> > functionLocalSymbols;
    map<string, vector<Symbol> > functionParams;
    //void addGlobal(const Symbol& sym);
    void addGlobal(int clazz, int type, const string& name);
    Symbol* getByName(const string& func, const string& name);
    void addLocal(const string& func, int clazz, int type, const string& name);
    vector<Symbol>& getParams(const string& func);
};
