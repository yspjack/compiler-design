add_executable(main lexer.cpp parser.cpp main.cpp symtab.cpp errproc.cpp
                IR.cpp codeGen.cpp optimize.cpp optCodeGen.cpp dataflow.cpp)
# add_library(main_lib lexer.cpp parser.cpp symtab.cpp errproc.cpp)
