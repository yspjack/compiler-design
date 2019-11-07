#include "IR.h"
#include <map>
#include <vector>
#include <string>
#include <sstream>

void IRCode::dump() const {
    static std::map<IROperator, std::string> IROperatorStr = {
        {IROperator::ADD, "+"},       {IROperator::SUB, "-"},
        {IROperator::MUL, "*"},       {IROperator::DIV, "/"},
        {IROperator::EQU, "=="},      {IROperator::NEQ, "!="},
        {IROperator::LEQ, "<="},      {IROperator::GEQ, ">="},
        {IROperator::LT, "<"},        {IROperator::GT, ">"},
        {IROperator::BNZ, "BNZ"},     {IROperator::BZ, "BZ"},
        {IROperator::JUMP, "GOTO"},   {IROperator::READ, "scanf"},
        {IROperator::WRITE, "printf"}};
    switch (op) {
    case IROperator::FUNC:
        printf("%s %s()\n", op2.c_str(), op1.c_str());
        break;
    case IROperator::PARA:
        printf("para %s %s\n", op2.c_str(), op1.c_str());
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
        if (dst != "")
            printf("var %s %s[%s]\n", op2.c_str(), op1.c_str(), dst.c_str());
        else
            printf("var %s %s\n", op2.c_str(), op1.c_str());
        break;
    case IROperator::CONSTANT:
        printf("const %s %s = %s\n", op2.c_str(), op1.c_str(), dst.c_str());
        break;
    case IROperator::LABEL:
        printf("%s:\n", op1.c_str());
        break;
    case IROperator::JUMP:
    case IROperator::BZ:
    case IROperator::BNZ:
        printf("%s %s %s\n", IROperatorStr[op].c_str(), op1.c_str(),
               op2.c_str());
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
    // case IROperator::LI:
    //     printf("%s = %s\n", dst.c_str(), op1.c_str());
    //     break;
    default:
        if (IROperatorStr.count(op)) {
            printf("%s %s %s %s\n", IROperatorStr[op].c_str(), op1.c_str(),
                   op2.c_str(), dst.c_str());
        } else {
            printf("%d %s %s %s\n", static_cast<int>(op), op1.c_str(),
                   op2.c_str(), dst.c_str());
        }
        break;
    }
}

std::string IRCode::dumpString() const {
    std::stringstream ss;
    static std::map<IROperator, std::string> IROperatorStr = {
        {IROperator::ADD, "+"},       {IROperator::SUB, "-"},
        {IROperator::MUL, "*"},       {IROperator::DIV, "/"},
        {IROperator::EQU, "=="},      {IROperator::NEQ, "!="},
        {IROperator::LEQ, "<="},      {IROperator::GEQ, ">="},
        {IROperator::LT, "<"},        {IROperator::GT, ">"},
        {IROperator::BNZ, "BNZ"},     {IROperator::BZ, "BZ"},
        {IROperator::JUMP, "GOTO"},   {IROperator::READ, "scanf"},
        {IROperator::WRITE, "printf"}};
    switch (op) {
    case IROperator::FUNC:
        ss << op2 << " " << op1 << "()";
        break;
    case IROperator::PARA:
        ss << "para " << op2 << " " << op1;
        break;
    case IROperator::CALL:
        ss << "call " << op1;
        break;
    case IROperator::PUSH:
        ss << "push " << op1 << " " << dst;
        break;
    case IROperator::RET:
        ss << "ret " << op1;
        break;
    case IROperator::VAR:
        if (dst != "") {
            ss << "var " << op2 << " " << op1 << "[" << dst << "]";
        } else {
            ss << "var " << op2 << " " << op1;
        }
        break;
    case IROperator::CONSTANT:
        ss << "const " << op2 << " " << op1 << " = " << dst;
        break;
    case IROperator::LABEL:
        ss << op1;
        break;
    case IROperator::JUMP:
    case IROperator::BZ:
    case IROperator::BNZ:
        ss << IROperatorStr[op] << " " << op1 << " " << op2;
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
        ss << dst << " = " << op1 << " " << IROperatorStr[op] << " " << op2;
        break;
    case IROperator::LOADARR:
        ss << dst << " = " << op1 << "[" << op2 << "]";
        break;
    case IROperator::SAVEARR:
        ss << op1 << "[" << op2 << "]"
           << " = " << dst;
        break;
    case IROperator::MOV:
        ss << dst << " = " << op1;
        break;
    default:
        if (IROperatorStr.count(op)) {
            ss << IROperatorStr[op] << " " << op1 << " " << op2 << " " << dst;
        } else {
            ss << static_cast<int>(op) << " " << op1 << " " << op2 << " "
               << dst;
        }
        break;
    }
    return ss.str();
}

IRCode::IRCode(IROperator op, const std::string &op1, const std::string &op2,
               const std::string &dst) {
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