#include "../include/tools/CircularString.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

// 简单的断言宏，用于测试
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "断言失败: " << message << " 在 " << __FILE__ << " 行 " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

// 测试构造函数
bool test_constructor() {
    // 测试正常构造
    CircularString cs(10);
    TEST_ASSERT(cs.size() == 0, "初始大小应为0");
    TEST_ASSERT(cs.cap() == 10, "容量应为10");
    
    // 测试异常情况
    try {
        CircularString invalid(0);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::invalid_argument& e) {
        // 预期的异常
    }
    
    return true;
}

// 测试push_back方法
bool test_push_back() {
    CircularString cs(5);
    
    // 测试未满时的插入
    cs.push_back("ABC");
    TEST_ASSERT(cs.size() == 3, "大小应为3");
    TEST_ASSERT(cs.substring(0, 2) == "ABC", "内容应为ABC");
    
    // 测试满时的插入（覆盖旧元素）
    cs.push_back("DEFG");
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "CDEFG", "内容应为CDEFG");
    
    return true;
}

// 测试find_nth方法
bool test_find_nth() {
    CircularString cs(10);
    cs.push_back("ABCABC");
    
    // 测试查找存在的字符
    TEST_ASSERT(cs.find_nth("A", 1) == 0, "第1个A应在位置0");
    TEST_ASSERT(cs.find_nth("A", 2) == 3, "第2个A应在位置3");
    
    // 测试查找不存在的字符
    try {
        cs.find_nth("Z", 1);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    // 测试查找超出次数的字符
    try {
        cs.find_nth("A", 3);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    return true;
}

// 测试substring方法
bool test_substring() {
    CircularString cs(10);
    cs.push_back("ABCDEFG");
    
    // 测试正常子串
    TEST_ASSERT(cs.substring(1, 3) == "BCD", "子串应为BCD");
    TEST_ASSERT(cs.substring(0, 6) == "ABCDEFG", "子串应为ABCDEFG");
    
    // 测试边界情况
    TEST_ASSERT(cs.substring(6, 6) == "G", "子串应为G");
    
    // 测试异常情况
    try {
        cs.substring(3, 2);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    try {
        cs.substring(0, 7);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    return true;
}

// 测试erase_up_to方法
bool test_erase_up_to() {
    CircularString cs(10);
    cs.push_back("ABCDEFG");
    
    // 测试正常删除
    cs.erase_up_to(2);
    TEST_ASSERT(cs.size() == 4, "大小应为4");
    TEST_ASSERT(cs.substring(0, 3) == "DEFG", "内容应为DEFG");
    
    // 测试删除后再插入
    cs.push_back("HIJ");
    TEST_ASSERT(cs.size() == 7, "大小应为7");
    TEST_ASSERT(cs.substring(0, 6) == "DEFGHIJ", "内容应为DEFGHIJ");
    
    // 测试异常情况
    try {
        cs.erase_up_to(7);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    return true;
}

// 测试环形覆盖逻辑
bool test_circular_logic() {
    CircularString cs(5);
    
    // 填满缓冲区
    cs.push_back("ABCDE");
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    
    // 添加更多字符，覆盖旧字符
    cs.push_back("FGH");
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "DEFGH", "内容应为DEFGH");
    
    // 删除部分字符
    cs.erase_up_to(1);
    TEST_ASSERT(cs.size() == 3, "大小应为3");
    TEST_ASSERT(cs.substring(0, 2) == "FGH", "内容应为FGH");
    
    // 再次添加字符
    cs.push_back("IJKLM");
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "IJKLM", "内容应为IJKLM");
    return true;
}

// 大规模测试（包含各种ASCII字符和字符串查找）
bool test_large_scale() {
    const size_t CAPACITY = 1500;
    CircularString cs(CAPACITY);
    
    // 生成测试数据（包含全部ASCII字符）
    std::string test_data;
    for(int i = 1; i < 256; ++i) {  // 从1开始，避免空字符
        if (i != '\r' && i != '\n') {  // 避免单独的\r和\n干扰测试
            test_data += static_cast<char>(i);
        }
    }
    
    // 重复填充数据直到超过容量
    const int REPEAT = 10;
    std::string big_data;
    for(int i = 0; i < REPEAT; ++i) {
        big_data += test_data + "\r\n";  // 使用实际的\r\n，而不是字符串"\\r\\n"
    }
    
    // 插入2000个字符（超过容量）
    cs.push_back(big_data);
    TEST_ASSERT(cs.size() == CAPACITY, "大小应等于容量");
    
    // 验证最后1500个字符正确
    std::string expected = big_data.substr(big_data.length() - CAPACITY);
    TEST_ASSERT(cs.substring(0, CAPACITY-1) == expected, "缓冲区内容不匹配");
    
    // 测试查找功能
    try {
        // 查找第5次出现的"\r\n"
        size_t pos = cs.find_nth("\r\n", 5);
        TEST_ASSERT(pos < CAPACITY - 2, "位置应在有效范围内");
        
        // 验证找到的位置确实匹配
        std::string found = cs.substring(pos, pos+1);
        TEST_ASSERT(found.length() == 2 && found[0] == '\r' && found[1] == '\n', "找到的字符串不匹配");
    } catch (const std::exception& e) {
        TEST_ASSERT(false, std::string("查找有效字符串时不应抛出异常: ") + e.what());
    }
    
    // 测试查找不存在的字符串
    try {
        cs.find_nth("这是一个肯定不存在的字符串@@##$$", 1);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        // 预期异常
    }
    
    return true;
}

// 运行所有测试
void run_all_tests() {
    struct {
        const char* name;
        bool (*test_func)();
    } tests[] = {
        {"构造函数测试", test_constructor},
        {"push_back测试", test_push_back},
        {"find_nth测试", test_find_nth},
        {"substring测试", test_substring},
        {"erase_up_to测试", test_erase_up_to},
        {"环形逻辑测试", test_circular_logic},
        {"大规模压力测试", test_large_scale} // 新增测试项
    };
    
    int passed = 0;
    int total = sizeof(tests) / sizeof(tests[0]);
    
    for (const auto& test : tests) {
        std::cout << "运行测试: " << test.name << "... ";
        if (test.test_func()) {
            std::cout << "通过" << std::endl;
            passed++;
        } else {
            std::cout << "失败" << std::endl;
        }
    }
    
    std::cout << "\n测试结果: " << passed << "/" << total << " 通过" << std::endl;
}

int main() {
    std::cout << "开始测试 CircularString 类...\n" << std::endl;
    run_all_tests();
    return 0;
}