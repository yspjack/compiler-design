# 收集所有以 test_ 开头的源文件
file(GLOB TEST_SOURCES "test_*.cpp")

# 为每个源文件创建可执行文件
foreach(SOURCE_FILE ${TEST_SOURCES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)  # 获取文件名（不包括扩展名）
    add_executable(${EXECUTABLE_NAME} ${SOURCE_FILE})
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE gtest gmock main_lib)
    add_test(NAME ${EXECUTABLE_NAME} COMMAND ${EXECUTABLE_NAME})
endforeach()