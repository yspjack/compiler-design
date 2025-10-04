#include <gtest/gtest.h>
#include "errproc.h"
#include "symtab.h"

TEST(SymTableTest, addGlobal_AllowedClass_Success) {
    SymTable symTable;
    symTable.addGlobal(Symbol::SYM_VAR, 0, "var1", 10);
    EXPECT_EQ(symTable.globalSymbols.size(), 1);
}

TEST(SymTableTest, addGlobal_NameRedefinition_ErrorHandled) {
    SymTable symTable;
    symTable.addGlobal(Symbol::SYM_VAR, 0, "var1", 10);
    symTable.addGlobal(Symbol::SYM_VAR, 0, "var1", 20);
}

TEST(SymTableTest, addGlobal_FunctionClass_InitializesFunctionMaps) {
    SymTable symTable;
    symTable.addGlobal(Symbol::SYM_FUNC, 0, "func1", 0);
    EXPECT_EQ(symTable.globalSymbols.size(), 1);
    EXPECT_EQ(symTable.functionLocalSymbols.count("func1"), 1);
    EXPECT_EQ(symTable.functionParams.count("func1"), 1);
}

TEST(SymTableTest, addGlobal_InvalidClass_AssertionFails) {
    SymTable symTable;
    EXPECT_DEATH(symTable.addGlobal(999, 0, "invalid", 0), "");
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
