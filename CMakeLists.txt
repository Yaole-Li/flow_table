cmake_minimum_required(VERSION 3.10)
project(flow_table VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加库文件
add_library(circular_string 
    src/tools/CircularString.cpp
)

# 添加测试可执行文件
add_executable(test_circular_string 
    test/test_circular_string.cpp
)

# 链接库
target_link_libraries(test_circular_string 
    circular_string
)

# 添加测试
enable_testing()
add_test(NAME CircularStringTest COMMAND test_circular_string)

# 安装规则
install(TARGETS circular_string
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ 
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 打印一些信息
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")