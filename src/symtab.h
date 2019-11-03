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
    int value;
    unsigned int addr;
    bool global;
    Symbol();
    Symbol(int clazz, int type, const string& name);
};


struct SymTable
{
    map<string, Symbol > globalSymbols;
    map<string, map<string, Symbol> > functionLocalSymbols;
    map<string, vector<Symbol> > functionParams;
    vector<string> stringPool;
    map<string, int> stringId;
    //void addGlobal(const Symbol& sym);
    void addGlobal(int clazz, int type, const string& name, int value = 0);
    Symbol* getByName(const string& func, const string& name);
    void addLocal(const string& func, int clazz, int type, const string& name, int value = 0);
    vector<Symbol>& getParams(const string& func);
    void addString(const string& value);
    void dump();
};
