cmake_minimum_required(VERSION 3.10)
project(flow_table VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 根据操作系统设置OpenSSL路径
set(OPENSSL_ROOT_DIR "/usr")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")

# 根据不同操作系统设置不同的库文件扩展名
if(APPLE)
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib/libssl.dylib" "${OPENSSL_ROOT_DIR}/lib/libcrypto.dylib")
else()
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib64/libssl.so" "${OPENSSL_ROOT_DIR}/lib64/libcrypto.so")
endif()

# 直接添加OpenSSL包含目录
include_directories(${OPENSSL_INCLUDE_DIR})
message(STATUS "Using OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "Using OpenSSL libraries: ${OPENSSL_LIBRARIES}")

# 查找libiconv库
find_library(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)
if(ICONV_LIBRARY)
    message(STATUS "Found libiconv: ${ICONV_LIBRARY}")
else()
    message(FATAL_ERROR "libiconv library not found")
endif()

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 定义libiconv的GNU接口
add_definitions(-DUSE_LIBICONV_GNU)

# 添加库文件
add_library(circular_string 
    src/tools/CircularString.cpp
)

# 添加s2c工具库
add_library(s2c_tools
    src/tools/s2ctools.cpp
)
target_link_libraries(s2c_tools ${OPENSSL_LIBRARIES} ${ICONV_LIBRARY})

# 添加流管理库
add_library(flow_manager
    src/flows/flow_manager.cpp
    src/flows/s2c_parser.cpp
)

# 链接流管理库与环形字符串库
target_link_libraries(flow_manager
    circular_string
    s2c_tools
    ${ICONV_LIBRARY}
)

# 添加测试可执行文件
add_executable(test_circular_string 
    test/test_circular_string.cpp
)

# 添加IMAP解析测试可执行文件
add_executable(test_imap_parsing
    test/test_imap_parsing.cpp
)

# 添加parseC2SData函数测试可执行文件
add_executable(test_parse_c2s_data
    test/test_parse_c2s_data.cpp
)

# 添加性能测试可执行文件
add_executable(test_imap_performance
    test/test_imap_performance.cpp
)

# 添加S2C解析器测试可执行文件
add_executable(test_s2c_parser
    test/test_s2c_parser.cpp
)

# 添加邮件关键词检测测试可执行文件
add_executable(test_email_keyword_detection
    test/test_email_keyword_detection.cpp
)

# 添加关键词检测器可执行文件
add_executable(keyword_detector
    extension/auto_AC/src/main.cpp
    extension/auto_AC/src/AhoCorasick.cpp
)

# 链接库
target_link_libraries(test_circular_string 
    circular_string
)

# 链接IMAP解析测试与流管理库
target_link_libraries(test_imap_parsing
    flow_manager
    ${ICONV_LIBRARY}
)

# 链接parseC2SData测试与流管理库
target_link_libraries(test_parse_c2s_data
    flow_manager
    ${ICONV_LIBRARY}
)

# 链接性能测试与流管理库
target_link_libraries(test_imap_performance
    flow_manager
    ${OPENSSL_LIBRARIES}
    ${ICONV_LIBRARY}
)

# 链接S2C解析器测试与流管理库
target_link_libraries(test_s2c_parser
    flow_manager
    s2c_tools
    ${OPENSSL_LIBRARIES}
    ${ICONV_LIBRARY}
)

# 链接邮件关键词检测测试与流管理库
target_link_libraries(test_email_keyword_detection
    flow_manager
    ${ICONV_LIBRARY}
)

# 链接关键词检测器与流管理库
target_link_libraries(keyword_detector
    flow_manager
    ${ICONV_LIBRARY}
)

# 添加测试
enable_testing()
add_test(NAME CircularStringTest COMMAND test_circular_string)
add_test(NAME ImapParsingTest COMMAND test_imap_parsing)
add_test(NAME ParseC2SDataTest COMMAND test_parse_c2s_data)
add_test(NAME ImapPerformanceTest COMMAND test_imap_performance)
add_test(NAME S2CParserTest COMMAND test_s2c_parser)
add_test(NAME EmailKeywordDetectionTest COMMAND test_email_keyword_detection)

# 安装规则
install(TARGETS circular_string flow_manager
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