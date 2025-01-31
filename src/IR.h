#pragma once
#include <string>
enum class IROperator {
    ADD,
    SUB,
    MUL,
    DIV,
    // NEG,
    LOADARR,
    SAVEARR,
    FUNC,
    PARA,
    CALL,
    RET,
    PUSH,
    VAR,
    CONSTANT,
    LEQ,
    LT,
    GEQ,
    GT,
    NEQ,
    EQU,
    JUMP,
    BNZ,
    BZ,
    LABEL,
    READ,
    WRITE,
    MOV
    // LI
};

struct IRCode {
    IROperator op;
    std::string op1, op2;
    std::string dst;
    IRCode(IROperator op, const std::string &op1, const std::string &op2, const std::string &dst);
    void dump() const;
    std::string dumpString() const;
};
