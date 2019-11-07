#ifndef PARSER_H
#define PARSER_H
#include "symtab.h"
#include "IR.h"
#include <vector>
#include <string>
using std::string;
using std::vector;
//<字符串>
extern void parse_string(string& tmp);
//<程序>
extern void program();
//<常量说明>
extern void constant_description();
//<常量定义>
extern void constant_definition();
//<无符号整数>
extern int unsigned_integer();
//<整数>
extern int integer();
//<声明头部>
//void declaration_header();
//<变量说明>
extern void variable_description();
//<变量定义>
extern void variable_definition();
//<有返回值函数定义>
extern void with_return_value_function_definition();
//<无返回值函数定义>
extern void no_return_value_function_definition();
//<复合语句>
extern void composite_statement();
//<参数表>
extern void parameter_table();
//<主函数>
extern void main_function();
//<表达式>
extern void expression(int& type,string&tmp);
//<项>
extern void item(int& type,string&tmp);
//<因子>
extern void factor(int& type,string&tmp);
//<语句>
extern void statement();
//<赋值语句>
extern void assignment_statement();
//<条件语句>
extern void conditional_statement();
//<条件>
extern void condition(string& tmp);
//<循环语句>
extern void loop_statement();
//<步长>
extern int step_size();
//<有返回值函数调用语句>
extern void with_return_value_function_call_statements(int& type);
//<无返回值函数调用语句>
extern void no_return_value_function_call_statement();
//<值参数表>
extern void value_parameter_table(const string& func);
//<语句列>
extern void statement_column();
//<读语句>
extern void read_statement();
//<写语句>
extern void write_statement();
//<返回语句>
extern void return_statement();

extern SymTable sym_table;
extern std::vector<IRCode> ircodes;
struct ParseState {
    string curFunc;
    bool foundReturn;
};
// extern ParseState context;
#endif