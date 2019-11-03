#include "IR.h"
#include <map>
#include <vector>
#include <string>

void IRCode::dump() {
    static std::map<IROperator, std::string> IROperatorStr = {
        {IROperator::ADD, "+"},    {IROperator::SUB, "-"},
        {IROperator::MUL, "*"},    {IROperator::DIV, "/"},
        {IROperator::EQU, "=="},   {IROperator::NEQ, "!="},
        {IROperator::LEQ, "<="},   {IROperator::GEQ, ">="},
        {IROperator::LT, "<"},     {IROperator::GT, ">"},
        {IROperator::BNZ, "BNZ"},  {IROperator::BZ, "BZ"},
        {IROperator::JUMP, "GOTO"}};
    switch (op) {
    case IROperator::FUNC:
        printf("%s %s()\n", op1.c_str(), op2.c_str());
        break;
    case IROperator::PARA:
        printf("para %s %s\n", op1.c_str(), op2.c_str());
        break;
    case IROperator::CALL:
        printf("call %s\n", op1.c_str());
        break;
    case IROperator::PUSH:
        printf("push %s\n", op1.c_str());
        break;
    case IROperator::RET:
        printf("ret %s\n", op1.c_str());
        break;
    case IROperator::VAR:
        printf("var %s %s\n", op1.c_str(), op2.c_str());
        break;
    case IROperator::CONSTANT:
        printf("const %s %s = %s\n", op1.c_str(), op2.c_str(), dst.c_str());
        break;
    case IROperator::LABEL:
        printf("%s\n", op1.c_str());
    case IROperator::JUMP:
    case IROperator::BZ:
    case IROperator::BNZ:
        printf("%s %s\n", IROperatorStr[op].c_str(), dst.c_str());
        break;
    case IROperator::ADD:
    case IROperator::SUB:
    case IROperator::MUL:
    case IROperator::DIV:
    case IROperator::EQU:
    case IROperator::NEQ:
    case IROperator::LEQ:
    case IROperator::GEQ:
    case IROperator::LT:
    case IROperator::GT:
        printf("%s = %s %s %s\n", dst.c_str(), op1.c_str(),
               IROperatorStr[op].c_str(), op2.c_str());
        break;
    case IROperator::LOADARR:
        printf("%s = %s[%s]\n", dst.c_str(), op1.c_str(), op2.c_str());
        break;
    case IROperator::SAVEARR:
        printf("%s[%s] = %s\n", op1.c_str(), op2.c_str(), dst.c_str());
        break;
    case IROperator::MOV:
        printf("%s = %s\n", dst.c_str(), op1.c_str());
        break;
    default:
        printf("%d %s %s %s\n", static_cast<int>(op), dst.c_str(), op1.c_str(),
               op2.c_str());
    }
}

IRCode::IRCode(IROperator op, std::string op1, std::string op2,
               std::string dst) {
    this->op = op;
    this->op1 = op1;
    this->op2 = op2;
    this->dst = dst;
}

void testIRCode() {
    std::vector<IRCode> A;
    A.push_back(IRCode(IROperator::ADD, "a", "b", "t1"));
    A.push_back(IRCode(IROperator::SUB, "a", "b", "t1"));
    A.push_back(IRCode(IROperator::EQU, "a", "b", "t1"));
    A.push_back(IRCode(IROperator::GEQ, "a", "b", "t1"));
    A.push_back(IRCode(IROperator::LT, "a", "b", "t1"));
    for (auto i : A) {
        i.dump();
    }
}