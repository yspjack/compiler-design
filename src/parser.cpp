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


SymTable sym_table;

struct ParseState {
    string curFunc;
    bool foundReturn;
};

ParseState context;

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

//<字符串>
void parse_string()
{
    sym_table.addString(tokenVal);
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
    sym_table.dump();
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
                sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_INT, name, sign * atoi(tokenVal.c_str()));
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
                    sym_table.addLocal(context.curFunc, Symbol::SYM_CONST, Symbol::SYM_INT, name, sign * atoi(tokenVal.c_str()));
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
        if (s != nullptr && s->clazz == Symbol::SYM_ARRAY) {
            s->size = size;
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
            if (s != nullptr && s->clazz == Symbol::SYM_ARRAY) {
                s->size = size;
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
    //<声明头部>
    // inline declaration_header
    if (tokenType == INTTK || tokenType == CHARTK) {
        if (tokenType == INTTK) {
            type = Symbol::SYM_INT;
        }
        else if (tokenType == CHARTK) {
            type = Symbol::SYM_CHAR;
        }
        skip();
        name = tokenVal;
        check(IDENFR);
    }
    else {
        // ERR
        assert(0);
    }
    sym_table.addGlobal(Symbol::SYM_FUNC, type, name);
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
    check(RBRACE);
    //puts("<有返回值函数定义>");
}
//<无返回值函数定义>
void no_return_value_function_definition() {
    check(VOIDTK);
    string name = tokenVal;
    sym_table.addGlobal(Symbol::SYM_FUNC, Symbol::SYM_VOID, name);
    context.curFunc = name;

    check(IDENFR);
    check(LPARENT);
    parameter_table();
    check(RPARENT);
    check(LBRACE);
    composite_statement();
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
        }
    }
    //puts("<参数表>");
}
//<主函数>
void main_function() {
    context.curFunc = "main";
    sym_table.addGlobal(Symbol::SYM_FUNC, Symbol::SYM_VOID, "main");
    check(VOIDTK);
    check(MAINTK);
    check(LPARENT);
    check(RPARENT);
    check(LBRACE);
    composite_statement();
    check(RBRACE);
    //puts("<主函数>");
}

int convType(int typeA, int typeB) {
    assert(typeA == Symbol::SYM_CHAR || typeA == Symbol::SYM_INT);
    assert(typeB == Symbol::SYM_CHAR || typeB == Symbol::SYM_INT);
    if (typeA == typeB) {
        return typeA;
    }
    else {
        return Symbol::SYM_INT;
    }
}

//<表达式>
void expression(int& type) {
    int typeB;
    if (tokenType == PLUS) {
        check(PLUS);
    }
    else if (tokenType == MINU) {
        check(MINU);
    }
    item(type);
    if (tokenType == PLUS || tokenType == MINU) {
        while (tokenType == PLUS || tokenType == MINU) {
            skip();
            item(typeB);
            type = convType(type, typeB);
        }
    }
    else {
        // ERROR
    }
    //puts("<表达式>");
}
//<项>
void item(int& type) {
    int typeB;

    factor(type);
    if (tokenType == MULT || tokenType == DIV) {
        while (tokenType == MULT || tokenType == DIV) {
            skip();
            factor(typeB);
            type = convType(type, typeB);

        }
    }
    //puts("<项>");
}
//<因子>
void factor(int& type) {
    if (tokenType == IDENFR) {
        int look = lookAhead(1);
        if (look == LBRACK) {
            check(IDENFR);
            check(LBRACK);
            expression(type);
            if (type != Symbol::SYM_INT) {
                handleError(SUBSCRIPT_CAN_ONLY_BE_INTEGER_EXPRESSION, linenumber);
            }
            check(RBRACK);
        }
        else if (look == LPARENT) {
            with_return_value_function_call_statements(type);
        }
        else {
            Symbol* s = sym_table.getByName(context.curFunc, tokenVal);
            type = s->type;
            check(IDENFR);
        }
    }
    else if (tokenType == LPARENT) {
        check(LPARENT);
        expression(type);
        check(RPARENT);
    }
    else if (tokenType == CHARCON) {
        check(CHARCON);
        type = Symbol::SYM_CHAR;
    }
    else {
        integer();
        type = Symbol::SYM_INT;
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
    checkAssign(tokenVal);

    check(IDENFR);
    if (tokenType == LBRACK) {
        check(LBRACK);
        expression(type);
        if (type != Symbol::SYM_INT) {
            handleError(SUBSCRIPT_CAN_ONLY_BE_INTEGER_EXPRESSION, linenumber);
        }
        check(RBRACK);
    }
    check(ASSIGN);
    expression(type);
    //puts("<赋值语句>");
}
//<条件语句>
void conditional_statement() {
    check(IFTK);
    check(LPARENT);
    condition();
    check(RPARENT);
    statement();

    if (tokenType == ELSETK) {
        check(ELSETK);
        statement();

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
//<条件>
void condition() {
    int typeA, typeB;
    expression(typeA);
    if (isRelation(tokenType)) {
        skip();
        expression(typeB);
        if (typeA != typeB) {
            handleError(ILLEGAL_TYPE_IN_CONDITION, linenumber);
        }
    }
    //puts("<条件>");
}
//<循环语句>
void loop_statement() {
    int type;
    if (tokenType == WHILETK) {
        check(WHILETK);
        check(LPARENT);
        condition();
        check(RPARENT);
        statement();
    }
    else if (tokenType == FORTK) {
        check(FORTK);
        check(LPARENT);
        checkAssign(tokenVal);
        check(IDENFR);
        check(ASSIGN);
        expression(type);
        check(SEMICN);
        condition();
        check(SEMICN);
        checkAssign(tokenVal);
        check(IDENFR);
        check(ASSIGN);
        check(IDENFR);

        if (tokenType == PLUS || tokenType == MINU) {
            skip();
            step_size();
        }
        else {
            // ERROR
        }

        check(RPARENT);
        statement();
    }
    else if (tokenType == DOTK) {
        check(DOTK);
        statement();
        check(WHILETK);
        check(LPARENT);
        condition();
        check(RPARENT);
    }
    else {
        // ERROR
    }
    //puts("<循环语句>");
}
//<步长>
void step_size() {
    unsigned_integer();
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
    //puts("<有返回值函数调用语句>");
}
//<无返回值函数调用语句>
void no_return_value_function_call_statement() {
    string func = tokenVal;
    check(IDENFR);
    check(LPARENT);
    value_parameter_table(func);
    check(RPARENT);
    //puts("<无返回值函数调用语句>");
}
//<值参数表>
void value_parameter_table(const string& func) {
    int paramCount = 0;
    bool paramCountError = false;
    int type;

    std::vector<Symbol>& params = sym_table.getParams(func);
    if (tokenType != RPARENT) {
        expression(type);
        if (paramCount >= params.size()) {
            paramCountError = true;
        }
        else {
            if (type != params[paramCount].type) {
                handleError(PARAMETER_TYPE_MISMATCH, linenumber);
            }
        }
        paramCount++;



        while (tokenType == COMMA) {
            skip();
            expression(type);

            if (paramCount >= params.size()) {
                paramCountError = true;
            }
            else {
                if (type != params[paramCount].type) {
                    handleError(PARAMETER_TYPE_MISMATCH, linenumber);
                }
            }
            paramCount++;
        }
    }
    if (paramCount != params.size()) {
        paramCountError = true;
    }
    if (paramCountError) {
        handleError(PARAMETER_NUMBER_MISMATCH, linenumber);
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
    check(SCANFTK);
    check(LPARENT);
    check(IDENFR);
    if (tokenType == COMMA) {
        while (tokenType == COMMA) {
            check(COMMA);
            check(IDENFR);
        }
    }
    check(RPARENT);
    //puts("<读语句>");
}
//<写语句>
void write_statement() {
    int type;
    check(PRINTFTK);
    check(LPARENT);

    if (tokenType == STRCON) {
        parse_string();
        if (tokenType == COMMA) {
            skip();
            expression(type);
        }
    }
    else {
        expression(type);
    }
    check(RPARENT);
    //puts("<写语句>");
}
//<返回语句>
void return_statement() {
    int type;
    Symbol* p = sym_table.getByName("", context.curFunc);
    context.foundReturn = true;

    check(RETURNTK);
    if (tokenType == LPARENT) {
        check(LPARENT);
        expression(type);
        check(RPARENT);
        if (p->type == Symbol::SYM_VOID) {
            handleError(MISMATCHED_RETURN_STATEMENT_FOR_VOID, linenumber);
        }
        else if (type != p->type) {
            handleError(MISSING_RETURN_STATEMENT_OR_MISMATCH, linenumber);
        }
    }
    else {
        if (p->type != Symbol::SYM_VOID) {
            handleError(MISSING_RETURN_STATEMENT_OR_MISMATCH, linenumber);
        }
    }
    //puts("<返回语句>");
}
