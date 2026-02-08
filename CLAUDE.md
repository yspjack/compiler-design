# CLAUDE.md

本文档为在本代码仓库中使用 Claude Code (claude.ai/code) 提供指导。

## 项目概述

这是一个编译器课程项目，实现了一个类 C 语言的编译器。该编译器将源代码翻译成 MIPS 汇编语言，并支持多种优化技术。

## 开发命令

### 构建项目
项目使用 CMake 和 Make 进行构建：

```bash
# 配置构建（从项目根目录执行）
cmake -S . -B build

# 构建所有目标
cd build && make
```

### 运行测试
项目使用 Google Test 框架。测试会自动从 test 目录下名为 `test_*.cpp` 的文件构建：

```bash
# 构建并运行所有测试
cd build && make && ./test/test_symtab && ./test/test_lexer

# 运行特定测试
cd build && ./test/test_symtab
```

### 运行编译器
编译器从 `testfile.txt` 读取输入，并将错误信息输出到 `error.txt`。运行方法如下：

```bash
# 先构建
cd build && make

# 运行编译器
./src/main
```

主可执行文件从项目根目录的 `testfile.txt` 读取输入，并生成 MIPS 汇编输出。

## 高层架构

编译器遵循传统的多阶段架构：

1. **词法分析** (`lexer.cpp`, `lexer.h`):
   - 将输入源代码分词
   - 在 `TOKENTYPE` 枚举中定义标记类型
   - 使用 `initLexer()` 和 `nextToken()` 函数

2. **语法分析** (`parser.cpp`, `parser.h`):
   - 实现递归下降解析
   - 包含每个语法规则的函数（如 `program()`、`expression()`）
   - 在解析过程中维护符号表和中间表示

3. **符号表管理** (`symtab.cpp`, `symtab.h`):
   - `SymTable` 结构体管理全局和局部符号
   - 跟踪变量、函数、参数和常量
   - 提供 `addGlobal()`、`addLocal()` 和 `getByName()` 等方法

4. **中间表示** (`IR.cpp`, `IR.h`):
   - `IRCode` 结构体表示中间指令
   - 使用三地址码格式和各种操作符（ADD, SUB, MUL, DIV 等）
   - 在解析过程中生成并存储在全局 `ircodes` 向量中

5. **代码生成与优化**:
   - `codeGen.cpp`: 将 IR 翻译为 MIPS 汇编
   - `optimize.cpp` 和 `dataflow.cpp`: 实现各种优化
   - 通过 `config.h` 中的宏定义进行配置

6. **错误处理** (`errproc.cpp`, `errproc.h`):
   - 使用 `handleError()` 函数进行集中式错误处理
   - 在 `ERROR_TYPE` 枚举中定义错误类型
   - 将错误输出到 `error.txt`

## 关键配置

`config.h` 文件包含控制优化功能的预处理器宏定义：
- `USE_OPTIMIZE`: 启用整体优化
- `OPT_REG_ALLOC`: 启用寄存器分配优化
- `OPT_CONST`: 启用常量折叠
- `OPT_DAG`: 启用基于有向无环图(DAG)的优化
- `OPT_LIVE_DATAFLOW`: 启用活跃变量分析
- `OPT_GRAPH_COLOR`: 启用图着色进行寄存器分配

可以通过注释/取消注释来启用/禁用特定优化。

## 输入/输出文件
- 输入: `testfile.txt` (待编译的源代码)
- 输出: MIPS 汇编写入 `mips.txt` (由代码生成)
- 错误: 写入 `error.txt`
- 设计文档: 位于 `docs/` 目录，涵盖词法分析、语法分析、IR、代码生成和优化

## 测试方法
单元测试使用 Google Test 框架实现：
- 每个测试文件（如 `test_symtab.cpp`）包含 TEST 宏
- 测试专注于各个组件（符号表、词法分析器等）
- 测试可执行文件会自动生成相同的名字
- 测试文件中的 main 函数初始化 Google Test 框架

修改编译器时，请确保更新相应的测试，并验证所有现有测试继续通过。