#pragma once

// 错误类型
// 非法符号或不符合词法
// 名字重定义
// 未定义的名字
// 函数参数个数不匹配
// 函数参数类型不匹配
// 条件判断中出现不合法的类型
// 无返回值的函数存在不匹配的return语句
// 有返回值的函数缺少return语句或存在不匹配的return语句
// 数组元素的下标只能是整型表达式
// 不能改变常量的值
// 应为分号
// 应为右小括号’)’
// 应为右中括号’]’
// do - while语句中缺少while
// 常量定义中 = 后面只能是整型或字符型常量
enum ERROR_TYPE {
    ILLEGAL_SYMBOL,
    NAME_REDEFINITION,
    UNDEFINED_NAME,
    PARAMETER_NUMBER_MISMATCH,
    PARAMETER_TYPE_MISMATCH,
    ILLEGAL_TYPE_IN_CONDITION,
    MISMATCHED_RETURN_STATEMENT_FOR_VOID,
    MISSING_RETURN_STATEMENT_OR_MISMATCH,
    SUBSCRIPT_CAN_ONLY_BE_INTEGER_EXPRESSION,
    CHANGE_VALUE_OF_CONSTANT,
    EXPECT_SEMICOLON,
    EXPECT_RIGHT_PARENTHESIS,
    EXPECT_RIGHT_BRACKET,
    MISSING_WHILE,
    CONSTANT_DEFINITION_ERROR
};
void handleError(int errCode, int line);