# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.5)

project ("error_proc")
enable_testing()

set(gtest_force_shared_crt ON)
add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
include_directories ("src")
add_subdirectory ("googletest-1.16.0")
add_subdirectory ("src")
add_subdirectory ("test")
