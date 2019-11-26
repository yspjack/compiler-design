#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
//#define NDEBUG
#include <cassert>

#include <string>
#include <map>
#include <vector>

#include "lexer.h"
#include "parser.h"
#include "symtab.h"
#include "errproc.h"
#include "IR.h"
using std::vector;
using std::string;

SymTable sym_table;

ParseState context;
vector<IRCode> ircodes;

void skip()
{
    //printf("%s %s\n", TokenTypeString[tokenType], tokenVal.c_str());
    nextToken();
}


#define check(token) ((void)check_wrapper((token),__LINE__))

void check_wrapper(int token, int line)
{
    if (token != tokenType)
    {
        if (token == SEMICN) {
            handleError(ERROR_TYPE::EXPECT_SEMICOLON, lastline);
        }
        else if (token == RBRACK) {
            handleError(ERROR_TYPE::EXPECT_RIGHT_BRACKET, lastline);
        }
        else if (token == RPARENT) {
            handleError(ERROR_TYPE::EXPECT_RIGHT_PARENTHESIS, lastline);
        }
        else if (token == WHILETK) {
            handleError(ERROR_TYPE::MISSING_WHILE, lastline);
        }
        else {
            fprintf(stderr, "LINE %d: Expected %s, got %s\n", line, TokenTypeString[token], TokenTypeString[tokenType]);
            fflush(stderr);
            abort();
        }
    }
    else {
        //printf("%s %s\n", TokenTypeString[tokenType], tokenVal.c_str());
        nextToken();
    }
}

std::string genLabel()
{
    static int cnt = 0;
    cnt++;
    return ".LC" + std::to_string(cnt);
}

struct TmpGenerate{
    int cnt;
    std::string operator()() {
      cnt++;
      return "#t" + std::to_string(cnt);
    }
    TmpGenerate() { cnt = 0; }
    void reset() { cnt = 0; }
};

TmpGenerate genTmp;

//<字符串>
void parse_string(string& tmp)
{
    sym_table.addString(tokenVal);
    tmp = ".string_" + std::to_string(sym_table.stringId[tokenVal]);
    check(STRCON);
    //puts("<字符串>\n");
}
//＜程序＞    ::= ［＜常量说明＞］［＜变量说明＞］{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
void program() {
    if (tokenType == CONSTTK)
        constant_description();

    if (tokenType == INTTK || tokenType == CHARTK) {
        if (lookAhead(2) != LPARENT) {
            variable_description();
        }
    }

    while (tokenType == VOIDTK || tokenType == INTTK || tokenType == CHARTK) {
        if (tokenType == INTTK || tokenType == CHARTK) {
            if (lookAhead(2) == LPARENT) {
                with_return_value_function_definition();
            }
        }
        if (tokenType == VOIDTK) {
            if (lookAhead(1) != MAINTK) {
                no_return_value_function_definition();
            }
            else {
                main_function();
                break;
            }
        }
    }
    //puts("<程序>");
}
//<常量说明>
void constant_description() {
    check(CONSTTK);
    constant_definition();
    check(SEMICN);
    while (tokenType == CONSTTK)
    {
        check(CONSTTK);
        constant_definition();
        check(SEMICN);
    }
    //puts("<常量说明>");
}
//<常量定义>
void constant_definition() {
    string name;
    if (tokenType == INTTK) {
        while (tokenType == INTTK) {
            check(INTTK);
            name = tokenVal;
            check(IDENFR);
            check(ASSIGN);
            //integer();
            int sign = 1;
            if (tokenType == PLUS)
            {
                check(PLUS);
                sign = 1;
            }
            else if (tokenType == MINU) {
                check(MINU);
                sign = -1;
            }
            if (tokenType != INTCON) {
                handleError(ERROR_TYPE::CONSTANT_DEFINITION_ERROR, linenumber);
                while (tokenType != SEMICN && tokenType != COMMA && tokenType != END) {
                    skip();
                }
            }
            else {
                int val = sign * atoi(tokenVal.c_str());
                sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_INT, name, val);
                ircodes.push_back(IRCode(IROperator::CONSTANT, name, "int", std::to_string(val)));
                skip();
            }

            while (tokenType == COMMA) {
                check(COMMA);
                name = tokenVal;
                check(IDENFR);
                check(ASSIGN);
                //integer();
                sign = 1;
                if (tokenType == PLUS)
                {
                    check(PLUS);
                    sign = 1;
                }
                else if (tokenType == MINU) {
                    check(MINU);
                    sign = -1;
                }
                if (tokenType != INTCON) {
                    handleError(ERROR_TYPE::CONSTANT_DEFINITION_ERROR, linenumber);
                    while (tokenType != SEMICN && tokenType != COMMA && tokenType != END) {
                        skip();
                    }
                }
                else {
                    int val = sign * atoi(tokenVal.c_str());
                    sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_INT, name, val);
                    ircodes.push_back(IRCode(IROperator::CONSTANT, name, "int", std::to_string(val)));
                    skip();
                }
            }
        }
    }
    else if (tokenType == CHARTK) {
        while (tokenType == CHARTK) {
            check(CHARTK);
            name = tokenVal;
            check(IDENFR);
            check(ASSIGN);

            if (tokenType != CHARCON) {
                handleError(ERROR_TYPE::CONSTANT_DEFINITION_ERROR, linenumber);
                while (tokenType != SEMICN && tokenType != COMMA && tokenType != END) {
                    skip();
                }
            }
            else {
                sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_CHAR, name, (int)tokenVal[0]);
                ircodes.push_back(IRCode(IROperator::CONSTANT, name, "char", std::to_string((int)tokenVal[0])));
                skip();
            }

            while (tokenType == COMMA) {
                skip();
                name = tokenVal;
                check(IDENFR);
                check(ASSIGN);
                if (tokenType != CHARCON) {
                    handleError(ERROR_TYPE::CONSTANT_DEFINITION_ERROR, linenumber);
                    while (tokenType != SEMICN && tokenType != COMMA && tokenType != END) {
                        skip();
                    }
                }
                else {
                    sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_CHAR, name, (int)tokenVal[0]);
                    ircodes.push_back(IRCode(IROperator::CONSTANT, name, "char", std::to_string((int)tokenVal[0])));
                    skip();
                }
            }
        }
    }
    else {
        handleError(ERROR_TYPE::CONSTANT_DEFINITION_ERROR, linenumber);
        while (tokenType != SEMICN && tokenType != COMMA && tokenType != END) {
            skip();
        }
        // ERROR
    }
    //puts("<常量定义>");
}
//<无符号整数>
int unsigned_integer() {
    string val = tokenVal;
    check(INTCON);
    //puts("<无符号整数>");
    return atoi(val.c_str());
}
//<整数>
int integer() {
    int sign = 1;
    if (tokenType == PLUS)
    {
        check(PLUS);
        sign = 1;
    }
    else if (tokenType == MINU) {
        check(MINU);
        sign = -1;
    }
    //puts("<整数>");
    return sign * unsigned_integer();
}

//<变量说明>
void variable_description() {
    variable_definition();
    check(SEMICN);
    //INTTK|CHARTK
    //IDENFR
    //...
    while (true) {
        if (tokenType != INTTK && tokenType != CHARTK) {
            break;
        }
        if (lookAhead(1) != IDENFR) {
            break;
        }
        if (lookAhead(2) == LPARENT) {
            break;
        }
        variable_definition();
        check(SEMICN);
    }
    //puts("<变量说明>");
}

//void single_variable() {
//    check(IDENFR);
//    if (tokenType == LBRACK) {
//        check(LBRACK);
//        unsigned_integer();
//        check(RBRACK);
//    }
//}
//<变量定义>
void variable_definition() {
    int kind;
    int type;
    int size;
    string name;
    if (tokenType == INTTK || tokenType == CHARTK) {
        kind = Symbol::SYM_VAR;
        size = 0;
        if (tokenType == INTTK) {
            type = Symbol::SYM_INT;
        }
        else if (tokenType == CHARTK) {
            type = Symbol::SYM_CHAR;
        }
        else {
            assert(0);
        }

        skip();
        name = tokenVal;
        check(IDENFR);
        if (tokenType == LBRACK) {
            kind = Symbol::SYM_ARRAY;
            check(LBRACK);
            size = unsigned_integer();
            check(RBRACK);
        }
        sym_table.addLocal(context.curFunc, kind, type, name);
        Symbol* s = sym_table.getByName(context.curFunc, name);
        assert(s != nullptr);
        if (s->clazz == Symbol::SYM_ARRAY) {
            s->size = size;
            ircodes.push_back(IRCode(IROperator::VAR, name,
                                     (type == Symbol::SYM_INT) ? "int" : "char", std::to_string(size)));
        } else {
            ircodes.push_back(IRCode(IROperator::VAR, name,
                                     (type == Symbol::SYM_INT) ? "int" : "char", ""));
        }

        while (tokenType == COMMA) {
            check(COMMA);
            kind = Symbol::SYM_VAR;
            size = 0;
            name = tokenVal;
            check(IDENFR);
            if (tokenType == LBRACK) {
                kind = Symbol::SYM_ARRAY;
                check(LBRACK);
                size = unsigned_integer();
                check(RBRACK);
            }
            sym_table.addLocal(context.curFunc, kind, type, name);
            Symbol* s = sym_table.getByName(context.curFunc, name);
            assert(s != nullptr);
            if (s->clazz == Symbol::SYM_ARRAY) {
                s->size = size;
                ircodes.push_back(IRCode(IROperator::VAR, name,
                                        (type == Symbol::SYM_INT) ? "int" : "char", std::to_string(size)));
            } else {
                ircodes.push_back(IRCode(IROperator::VAR, name,
                                        (type == Symbol::SYM_INT) ? "int" : "char", ""));
            }
        }
    }
    else {
        // ERROR
        assert(0);
    }
    //puts("<变量定义>");
}
//<有返回值函数定义>
void with_return_value_function_definition() {
    context.foundReturn = false;
    int type = -1;
    string name;
    IRCode code(IROperator::FUNC, "", "", "");
    //<声明头部>
    // inline declaration_header
    if (tokenType == INTTK || tokenType == CHARTK) {
        if (tokenType == INTTK) {
            type = Symbol::SYM_INT;
            code.op2 = "int";
        }
        else if (tokenType == CHARTK) {
            type = Symbol::SYM_CHAR;
            code.op2 = "char";
        }
        skip();
        name = tokenVal;
        code.op1=name;
        check(IDENFR);
    }
    else {
        // ERR
        assert(0);
    }
    sym_table.addGlobal(Symbol::SYM_FUNC, type, name);
    ircodes.push_back(code);
    context.curFunc = name;
    //puts("<声明头部>");

    check(LPARENT);
    parameter_table();
    check(RPARENT);
    check(LBRACE);
    composite_statement();
    if (!context.foundReturn) {
        handleError(MISSING_RETURN_STATEMENT_OR_MISMATCH, linenumber);
    }
    ircodes.push_back(IRCode(IROperator::RET,"0","",""));
    check(RBRACE);
    //puts("<有返回值函数定义>");
}
//<无返回值函数定义>
void no_return_value_function_definition() {
    check(VOIDTK);
    string name = tokenVal;
    sym_table.addGlobal(Symbol::SYM_FUNC, Symbol::SYM_VOID, name);
    ircodes.push_back(IRCode(IROperator::FUNC, name, "void", ""));
    context.curFunc = name;

    check(IDENFR);
    check(LPARENT);
    parameter_table();
    check(RPARENT);
    check(LBRACE);
    composite_statement();
    ircodes.push_back(IRCode(IROperator::RET,"","",""));
    check(RBRACE);

    //puts("<无返回值函数定义>");
}
//<复合语句>
void composite_statement() {
    if (tokenType == CONSTTK) {
        constant_description();
    }
    if (tokenType == INTTK || tokenType == CHARTK) {
        variable_description();
    }
    {
        statement_column();
    }
    //puts("<复合语句>");
}
//<参数表>
void parameter_table() {
    int type;
    string name;

    if (tokenType == INTTK || tokenType == CHARTK) {
        assert(tokenType == INTTK || tokenType == CHARTK);
        if (tokenType == INTTK) {
            type = Symbol::SYM_INT;
        }
        else if (tokenType == CHARTK) {
            type = Symbol::SYM_CHAR;
        }
        else {
            assert(0);
        }
        skip();
        name = tokenVal;
        check(IDENFR);
        sym_table.addLocal(context.curFunc, Symbol::SYM_PARAM, type, name);
        ircodes.push_back(IRCode(IROperator::PARA, name,
                                 ((type == Symbol::SYM_INT) ? "int" : "char"),
                                 ""));

        while (tokenType == COMMA) {
            check(COMMA);
            assert(tokenType == INTTK || tokenType == CHARTK);
            if (tokenType == INTTK) {
                type = Symbol::SYM_INT;
            }
            else if (tokenType == CHARTK) {
                type = Symbol::SYM_CHAR;
            }
            else {
                assert(0);
            }
            skip();
            name = tokenVal;
            check(IDENFR);
            sym_table.addLocal(context.curFunc, Symbol::SYM_PARAM, type, name);
            ircodes.push_back(
                IRCode(IROperator::PARA, name,
                       ((type == Symbol::SYM_INT) ? "int" : "char"), ""));
        }
    }
    //puts("<参数表>");
}
//<主函数>
void main_function() {
    context.curFunc = "main";
    sym_table.addGlobal(Symbol::SYM_FUNC, Symbol::SYM_VOID, "main");
    // ircodes.push_back(IRCode(IROperator::LABEL,"main","",""));
    ircodes.push_back(IRCode(IROperator::FUNC, "main", "void", ""));
    check(VOIDTK);
    check(MAINTK);
    check(LPARENT);
    check(RPARENT);
    check(LBRACE);
    composite_statement();
    ircodes.push_back(IRCode(IROperator::RET,"","",""));
    check(RBRACE);
    //puts("<主函数>");
}

int convType(int typeA, int typeB) {
    assert(typeA == Symbol::SYM_CHAR || typeA == Symbol::SYM_INT);
    assert(typeB == Symbol::SYM_CHAR || typeB == Symbol::SYM_INT);
    // if (typeA == typeB) {
    //     return typeA;
    // }
    // else {
    //     return Symbol::SYM_INT;
    // }
    return Symbol::SYM_INT;
}

//<表达式>
void expression(int& type,string&tmp) {
    int typeB;
    string tmpB;
    bool neg=false;
    if (tokenType == PLUS) {
        check(PLUS);neg=false;
    }
    else if (tokenType == MINU) {
        check(MINU);neg=true;
    }
    item(type, tmp);
    if (neg) {
        // ircodes.push_back(IRCode(IROperator::NEG, tmpA, "", ""));
        string tmpA = tmp;
        tmp = genTmp();
        type = Symbol::SYM_INT;
        ircodes.push_back(IRCode(IROperator::SUB, std::to_string(0), tmpA, tmp));
    }
    if (tokenType == PLUS || tokenType == MINU) {
        while (tokenType == PLUS || tokenType == MINU) {
            int opType=tokenType;
            skip();
            item(typeB, tmpB);
            type = convType(type, typeB);
            IROperator op=(opType == PLUS) ? IROperator::ADD:IROperator::SUB;
            string tmpA = tmp;
            tmp=genTmp();
            ircodes.push_back(IRCode(op, tmpA, tmpB, tmp));
        }
    }
    else {
        // ERROR
    }
    //puts("<表达式>");
}
//<项>
void item(int& type,string&tmp) {
    int typeB;
    string tA,tB;
    

    factor(type,tA);
    tmp=tA;
    if (tokenType == MULT || tokenType == DIV) {
        while (tokenType == MULT || tokenType == DIV) {
            int opType=tokenType;
            skip();
            factor(typeB,tB);
            type = convType(type, typeB);
            IROperator op=(opType == MULT) ? IROperator::MUL:IROperator::DIV;
            tmp=genTmp();
            ircodes.push_back(IRCode(op,tA,tB,tmp));
            tA=tmp;
        }
    }
    //puts("<项>");
}
//<因子>
void factor(int& type,string&tmp) {
    string tA,tB;
    if (tokenType == IDENFR) {
        int look = lookAhead(1);
        if (look == LBRACK) {
            tA=tokenVal;
            check(IDENFR);
            check(LBRACK);
            expression(type,tB);
            if (type != Symbol::SYM_INT) {
                handleError(SUBSCRIPT_CAN_ONLY_BE_INTEGER_EXPRESSION, linenumber);
            }
            check(RBRACK);
            Symbol *s = sym_table.getByName(context.curFunc, tA);
            type = s->type;
            tmp=genTmp();
            ircodes.push_back(IRCode(IROperator::LOADARR,tA,tB,tmp));
        }
        else if (look == LPARENT) {
            with_return_value_function_call_statements(type);
            tmp=genTmp();
            ircodes.push_back(IRCode(IROperator::MOV,"#RET","",tmp));
        }
        else {
            Symbol* s = sym_table.getByName(context.curFunc, tokenVal);
            type = s->type;
            if (s->clazz == Symbol::SYM_CONST) {
                // tmp = genTmp();
                // ircodes.push_back(
                //     IRCode(IROperator::LI, std::to_string(s->value), "", tmp));
                tmp=std::to_string(s->value);
            } else {
                tmp = tokenVal;
            }
            check(IDENFR);
        }
    }
    else if (tokenType == LPARENT) {
        check(LPARENT);
        expression(type,tmp);
        type = Symbol::SYM_INT;
        check(RPARENT);
    }
    else if (tokenType == CHARCON) {
        int val = static_cast<int>(tokenVal[0]);
        check(CHARCON);
        type = Symbol::SYM_CHAR;
        // tmp = genTmp();
        // ircodes.push_back(IRCode(IROperator::LI, std::to_string(val), "", tmp));
        tmp=std::to_string(val);
    }
    else {
        int val = integer();
        type = Symbol::SYM_INT;
        // tmp = genTmp();
        // ircodes.push_back(IRCode(IROperator::LI, std::to_string(val), "", tmp));
        tmp=std::to_string(val);
    }
    //puts("<因子>");
}
//<语句>
void statement() {
    if (tokenType == IFTK) {
        conditional_statement();
    }
    else if (tokenType == FORTK
        || tokenType == DOTK
        || tokenType == WHILETK) {
        loop_statement();
    }
    else if (tokenType == LBRACE) {
        check(LBRACE);
        statement_column();
        check(RBRACE);
    }
    else if (tokenType == SCANFTK) {
        read_statement();
        check(SEMICN);
    }
    else if (tokenType == PRINTFTK) {
        write_statement();
        check(SEMICN);
    }
    else if (tokenType == RETURNTK) {
        return_statement();
        check(SEMICN);
    }
    else if (tokenType == IDENFR) {
        std::string id = tokenVal;
        int look = lookAhead(1);
        if (look == LPARENT) {
            Symbol* psym = sym_table.getByName("", id);
            if (psym == nullptr) {
                handleError(UNDEFINED_NAME, linenumber);
                while (tokenType != SEMICN && tokenType != RPARENT && tokenType != END) {
                    skip();
                }
                check(RPARENT);
            }
            else {
                Symbol sym = *psym;
                assert(sym.clazz == Symbol::SYM_FUNC);
                if (sym.type == Symbol::SYM_INT || sym.type == Symbol::SYM_CHAR) {
                    int type;
                    with_return_value_function_call_statements(type);
                }
                else if (sym.type == Symbol::SYM_VOID)
                    no_return_value_function_call_statement();
                else {
                    //ERR
                    assert(0);
                }
            }
        }
        else {
            assignment_statement();
        }

        check(SEMICN);
    }
    else {
        check(SEMICN);
    }
    //puts("<语句>");
}

void checkAssign(const string& name)
{
    Symbol* s = sym_table.getByName(context.curFunc, name);
    if (s == nullptr) {
        handleError(UNDEFINED_NAME, linenumber);
    }
    else {
        if (s->clazz == Symbol::SYM_CONST) {
            handleError(CHANGE_VALUE_OF_CONSTANT, linenumber);
        }
    }
}

//<赋值语句>
void assignment_statement() {
    int type;
    string tA, tB, tmp;
    bool isArr=false;
    checkAssign(tokenVal);

    tA = tokenVal;
    check(IDENFR);
    if (tokenType == LBRACK) {
        check(LBRACK);
        expression(type, tB);
        if (type != Symbol::SYM_INT) {
            handleError(SUBSCRIPT_CAN_ONLY_BE_INTEGER_EXPRESSION, linenumber);
        }
        check(RBRACK);
        isArr=true;
    }
    check(ASSIGN);
    expression(type, tmp);
    if(isArr){
        ircodes.push_back(IRCode(IROperator::SAVEARR, tA, tB, tmp));
    }else{
        ircodes.push_back(IRCode(IROperator::MOV, tmp, "", tA));
    }
    // puts("<赋值语句>");
}
//<条件语句>
void conditional_statement() {
    string tmp;
    string labA, labB;
    check(IFTK);
    check(LPARENT);
    condition(tmp);
    check(RPARENT);
    labA = genLabel();
    ircodes.push_back(IRCode(IROperator::BZ, tmp, labA, ""));
    statement();
    if (tokenType == ELSETK) {
        labB = genLabel();
        ircodes.push_back(IRCode(IROperator::JUMP,  labB, "",""));
        ircodes.push_back(IRCode(IROperator::LABEL, labA, "", ""));
        check(ELSETK);
        //ircodes.push_back(IRCode(IROperator::BNZ, tmp, labB, ""));
        statement();
        ircodes.push_back(IRCode(IROperator::LABEL, labB, "", ""));
    }
    else {
        ircodes.push_back(IRCode(IROperator::LABEL, labA, "", ""));
    }
    //puts("<条件语句>");
}
bool isRelation(int t) {
    switch (t)
    {
    case LSS:
    case LEQ:
    case GRE:
    case GEQ:
    case EQL:
    case NEQ:
        return true;
    }
    return false;
}
IROperator getRelationOp(int t)
{
    switch (t) {
    case LSS:
        return IROperator::LT;
    case LEQ:
        return IROperator::LEQ;
    case GRE:
        return IROperator::GT;
    case GEQ:
        return IROperator::GEQ;
    case EQL:
        return IROperator::EQU;
    case NEQ:
        return IROperator::NEQ;
    default:
        assert(0);
    }
}
//<条件>
void condition(string &dst) {
    IROperator op;
    int typeA, typeB;
    string tmpA, tmpB;
    // dst="$FLAG";
    dst=genTmp();
    expression(typeA, tmpA);
    if (isRelation(tokenType)) {
        op = getRelationOp(tokenType);
        skip();
        expression(typeB, tmpB);
        if (typeA != typeB) {
            handleError(ILLEGAL_TYPE_IN_CONDITION, linenumber);
        }
        ircodes.push_back(IRCode(op, tmpA, tmpB, dst));
    }else{
        ircodes.push_back(IRCode(IROperator::NEQ, tmpA, "0", dst));
    }
    // puts("<条件>");
}
//<循环语句>
void loop_statement() {
    int type;
    string tmp;
    if (tokenType == WHILETK) {
        string labA, labB;
        labA = genLabel();
        labB = genLabel();
        check(WHILETK);
        ircodes.push_back(IRCode(IROperator::LABEL, labA, "", ""));
        check(LPARENT);
        condition(tmp);
        check(RPARENT);
        ircodes.push_back(IRCode(IROperator::BZ, tmp, labB, ""));
        statement();
        ircodes.push_back(IRCode(IROperator::JUMP, labA, "", ""));
        ircodes.push_back(IRCode(IROperator::LABEL, labB, "", ""));
    }
    else if (tokenType == FORTK) {
        // string labStep = genLabel();
        string labCompare = genLabel();
        // string labLoop = genLabel();
        string labEnd = genLabel();
        string loopVar, loopVar2;
        int step;
        check(FORTK);
        check(LPARENT);
        loopVar = tokenVal;
        checkAssign(tokenVal);
        check(IDENFR);
        check(ASSIGN);
        expression(type, tmp);
        ircodes.push_back(IRCode(IROperator::MOV, tmp, "", loopVar));
        // ircodes.push_back(IRCode(IROperator::JUMP, labCompare, "", ""));
        check(SEMICN);
        ircodes.push_back(IRCode(IROperator::LABEL, labCompare, "", ""));
        condition(tmp);
        check(SEMICN);
        // ircodes.push_back(IRCode(IROperator::LABEL, labLoop, "", ""));
        loopVar = tokenVal;
        checkAssign(tokenVal);
        check(IDENFR);
        check(ASSIGN);
        loopVar2 = tokenVal;
        check(IDENFR);
        IROperator op;
        if (tokenType == PLUS || tokenType == MINU) {
          op = (tokenType == PLUS) ? IROperator::ADD : IROperator::SUB;
          skip();
          step = step_size();
        } else {
          // ERROR
        }

        check(RPARENT);
        ircodes.push_back(IRCode(IROperator::BZ, tmp, labEnd, ""));
        statement();

        // tmp = genTmp();
        // ircodes.push_back(
        //     IRCode(IROperator::LI, std::to_string(step), "", tmp));
        tmp=std::to_string(step);
        ircodes.push_back(IRCode(op, loopVar2, tmp, loopVar));

        ircodes.push_back(IRCode(IROperator::JUMP, labCompare, "", ""));
        ircodes.push_back(IRCode(IROperator::LABEL, labEnd, "", ""));
    }
    else if (tokenType == DOTK) {
        string labA;
        labA = genLabel();
        check(DOTK);
        ircodes.push_back(IRCode(IROperator::LABEL, labA, "", ""));
        statement();
        check(WHILETK);
        check(LPARENT);
        condition(tmp);
        check(RPARENT);
        ircodes.push_back(IRCode(IROperator::BNZ, tmp, labA, ""));
    }
    else {
        // ERROR
    }
    //puts("<循环语句>");
}
//<步长>
int step_size() {
    return unsigned_integer();
    //puts("<步长>");
}
//<有返回值函数调用语句>
void with_return_value_function_call_statements(int& type) {
    string func = tokenVal;
    Symbol* s = sym_table.getByName(context.curFunc, func);
    type = s->type;
    check(IDENFR);
    check(LPARENT);
    value_parameter_table(func);
    check(RPARENT);
    ircodes.push_back(IRCode(IROperator::CALL,func,"",""));
    //puts("<有返回值函数调用语句>");
}
//<无返回值函数调用语句>
void no_return_value_function_call_statement() {
    string func = tokenVal;
    check(IDENFR);
    check(LPARENT);
    value_parameter_table(func);
    check(RPARENT);
    ircodes.push_back(IRCode(IROperator::CALL,func,"",""));
    //puts("<无返回值函数调用语句>");
}
//<值参数表>
void value_parameter_table(const string& func) {
    int paramCount = 0;
    bool paramCountError = false;
    int type;
    string tmp;
    vector<IRCode> pushCode;
    auto getType = [](int t) {
        switch (t) {
        case Symbol::SYM_INT:
            return "INT";
        case Symbol::SYM_CHAR:
            return "CHAR";
        default:
            return "";
        }
        return "";
    };

    std::vector<Symbol>& params = sym_table.getParams(func);
    if (tokenType != RPARENT) {
        expression(type,tmp);
        if (paramCount >= params.size()) {
            paramCountError = true;
        }
        else {
            if (type != params[paramCount].type) {
                handleError(PARAMETER_TYPE_MISMATCH, linenumber);
            }
        }
        pushCode.push_back(IRCode(IROperator::PUSH, tmp, std::to_string(paramCount), func));
        paramCount++;

        while (tokenType == COMMA) {
            skip();
            expression(type,tmp);

            if (paramCount >= params.size()) {
                paramCountError = true;
            }
            else {
                if (type != params[paramCount].type) {
                    handleError(PARAMETER_TYPE_MISMATCH, linenumber);
                }
            }
            pushCode.push_back(IRCode(IROperator::PUSH, tmp, std::to_string(paramCount), func));
            paramCount++;
        }
    }
    if (paramCount != params.size()) {
        paramCountError = true;
    }
    if (paramCountError) {
        handleError(PARAMETER_NUMBER_MISMATCH, linenumber);
    }
    for (int i = 0; i < pushCode.size(); ++i) {
        ircodes.push_back(pushCode[i]);
    }

    //puts("<值参数表>");
}
//<语句列>
void statement_column() {
    while (tokenType != RBRACE) {
        statement();
    }
    //puts("<语句列>");
}
//<读语句>
void read_statement() {
    Symbol *s;
    check(SCANFTK);
    check(LPARENT);
    s = sym_table.getByName(context.curFunc, tokenVal);
    assert((s->type == Symbol::SYM_INT) || (s->type == Symbol::SYM_CHAR));
    string type = (s->type == Symbol::SYM_INT) ? "INT" : "CHAR";
    ircodes.push_back(IRCode(IROperator::READ, s->name, type, ""));
    check(IDENFR);
    if (tokenType == COMMA) {
        while (tokenType == COMMA) {
            check(COMMA);
            s = sym_table.getByName(context.curFunc, tokenVal);
            assert((s->type == Symbol::SYM_INT) ||
                   (s->type == Symbol::SYM_CHAR));
            type = (s->type == Symbol::SYM_INT) ? "INT" : "CHAR";
            ircodes.push_back(IRCode(IROperator::READ, s->name, type, ""));
            check(IDENFR);
        }
    }
    check(RPARENT);
    //puts("<读语句>");
}
//<写语句>
void write_statement() {
    int type;
    string tmp;
    check(PRINTFTK);
    check(LPARENT);
    auto getType = [](int t) {
        switch (t) {
        case Symbol::SYM_INT:
            return "INT";
        case Symbol::SYM_CHAR:
            return "CHAR";
        default:
            return "";
        }
        return "";
    };

    if (tokenType == STRCON) {
        parse_string(tmp);
        ircodes.push_back(IRCode(IROperator::WRITE,tmp,"STR",""));
        if (tokenType == COMMA) {
            skip();
            expression(type,tmp);
            ircodes.push_back(IRCode(IROperator::WRITE,tmp,getType(type),""));
        }
    }
    else {
        expression(type,tmp);
        IROperator op = (type == Symbol::SYM_INT) ? (IROperator::WRITE)
                                                  : (IROperator::WRITE);
        ircodes.push_back(IRCode(op,tmp,getType(type),""));
    }
    check(RPARENT);
    ircodes.push_back(IRCode(IROperator::WRITE, std::to_string((int)'\n'), "CHAR", ""));
    //puts("<写语句>");
}
//<返回语句>
void return_statement() {
    int type;
    Symbol* p = sym_table.getByName("", context.curFunc);
    context.foundReturn = true;

    check(RETURNTK);
    if (tokenType == LPARENT) {
        string tmp;
        check(LPARENT);
        expression(type,tmp);
        check(RPARENT);
        if (p->type == Symbol::SYM_VOID) {
            handleError(MISMATCHED_RETURN_STATEMENT_FOR_VOID, linenumber);
        }
        else if (type != p->type) {
            handleError(MISSING_RETURN_STATEMENT_OR_MISMATCH, linenumber);
        }
        ircodes.push_back(IRCode(IROperator::RET,tmp,"",""));
    }
    else {
        if (p->type != Symbol::SYM_VOID) {
            handleError(MISSING_RETURN_STATEMENT_OR_MISMATCH, linenumber);
        }
        ircodes.push_back(IRCode(IROperator::RET,"","",""));
    }
    //puts("<返回语句>");
}
