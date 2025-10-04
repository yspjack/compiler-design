#include <gtest/gtest.h>
#include "lexer.h"

TEST(LexerTest, test_empty) {
    // 测试空白字符
    std::string buf = "   \n";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, END);
}

TEST(LexerTest, test_keyword) {
    std::string buf = "const int char void main if else do while for scanf printf return";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);

    nextToken();
    EXPECT_EQ(tokenType, CONSTTK);
    nextToken();
    EXPECT_EQ(tokenType, INTTK);
    nextToken();
    EXPECT_EQ(tokenType, CHARTK);
    nextToken();
    EXPECT_EQ(tokenType, VOIDTK);
    nextToken();
    EXPECT_EQ(tokenType, MAINTK);
    nextToken();
    EXPECT_EQ(tokenType, IFTK);
    nextToken();
    EXPECT_EQ(tokenType, ELSETK);
    nextToken();
    EXPECT_EQ(tokenType, DOTK);
    nextToken();
    EXPECT_EQ(tokenType, WHILETK);
    nextToken();
    EXPECT_EQ(tokenType, FORTK);
    nextToken();
    EXPECT_EQ(tokenType, SCANFTK);
    nextToken();
    EXPECT_EQ(tokenType, PRINTFTK);
    nextToken();
    EXPECT_EQ(tokenType, RETURNTK);
}

TEST(LexerTest, test_identifier) {
    // 测试标识符
    std::string buf = "identifier";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, IDENFR);
}

TEST(LexerTest, test_digit) {
    // 测试数字
    std::string buf = "123 012";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, INTCON);
    nextToken();
    EXPECT_EQ(tokenType, INTCON);
}

TEST(LexerTest, test_char) {
    // 测试字符
    std::string buf = "'a' '1' ' '";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, CHARCON);
    nextToken();
    EXPECT_EQ(tokenType, CHARCON);
    nextToken();
    EXPECT_EQ(tokenType, CHARCON);
}

TEST(LexerTest, test_string) {
    // 测试字符串
    std::string buf = "\"string\" \"123\"";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, STRCON);
    nextToken();
    EXPECT_EQ(tokenType, STRCON);
}

TEST(LexerTest, test_operator) {
    // 测试运算符和标点符号
    std::string buf = "+ - * / ; , ( ) [ ] { } > >= < <= != = ==";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, PLUS);
    nextToken();
    EXPECT_EQ(tokenType, MINU);
    nextToken();
    EXPECT_EQ(tokenType, MULT);
    nextToken();
    EXPECT_EQ(tokenType, DIV);
    nextToken();
    EXPECT_EQ(tokenType, SEMICN);
    nextToken();
    EXPECT_EQ(tokenType, COMMA);
    nextToken();
    EXPECT_EQ(tokenType, LPARENT);
    nextToken();
    EXPECT_EQ(tokenType, RPARENT);
    nextToken();
    EXPECT_EQ(tokenType, LBRACK);
    nextToken();
    EXPECT_EQ(tokenType, RBRACK);
    nextToken();
    EXPECT_EQ(tokenType, LBRACE);
    nextToken();
    EXPECT_EQ(tokenType, RBRACE);
    nextToken();
    EXPECT_EQ(tokenType, GRE);
    nextToken();
    EXPECT_EQ(tokenType, GEQ);
    nextToken();
    EXPECT_EQ(tokenType, LSS);
    nextToken();
    EXPECT_EQ(tokenType, LEQ);
    nextToken();
    EXPECT_EQ(tokenType, NEQ);
    nextToken();
    EXPECT_EQ(tokenType, ASSIGN);
    nextToken();
    EXPECT_EQ(tokenType, EQL);
}

TEST(LexerTest, test_unknown_char) {
    // 测试未知字符
    std::string buf = "@";
    size_t n = buf.size();
    initLexer(buf.c_str(), n);
    nextToken();
    EXPECT_EQ(tokenType, UKN);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
