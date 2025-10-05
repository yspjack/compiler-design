# 词法分析

## 单词类别

单词的类别码按如下形式定义

| 单词名称 | 类别码  | 单词名称 | 类别码   |
| -------- | ------- | -------- | -------- |
| 标识符   | IDENFR  | if       | IFTK     |
| 整型常量 | INTCON  | else     | ELSETK   |
| 字符常量 | CHARCON | do       | DOTK     |
| 字符串   | STRCON  | while    | WHILETK  |
| const    | CONSTTK | for      | FORTK    |
| int      | INTTK   | scanf    | SCANFTK  |
| char     | CHARTK  | printf   | PRINTFTK |
| void     | VOIDTK  | return   | RETURNTK |
| main     | MAINTK  | +        | PLUS     |
| -        | MINU    | `=`      | ASSIGN   |
| `*`      | MULT    | ;        | SEMICN   |
| `/`      | DIV     | ,        | COMMA    |
| `<`      | LSS     | (        | LPARENT  |
| `<=`     | LEQ     | )        | RPARENT  |
| `>`      | GRE     | \[       | LBRACK   |
| `>=`     | GEQ     | ]        | RBRACK   |
| `==`     | EQL     | {        | LBRACE   |
| `!=`     | NEQ     | }        | RBRACE   |
