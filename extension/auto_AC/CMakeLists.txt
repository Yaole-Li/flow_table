cmake_minimum_required(VERSION 3.10)
project(SensitiveWordDetector)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(detector
    src/main.cpp
    src/AhoCorasick.cpp
    src/base64.cpp
)

target_include_directories(detector PUBLIC include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(detector PRIVATE -O2)
endif()