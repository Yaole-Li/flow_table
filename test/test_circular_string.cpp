/**
 * @file test_circular_string.cpp
 * @brief 环形字符串类测试
 * 
 * 本测试文件用于验证环形字符串(CircularString)类的功能正确性。
 * 环形字符串是IMAP协议解析的基础数据结构，用于高效处理网络数据流。
 * 
 * 主要测试功能：
 * 1. 基本构造函数测试 - 验证初始化和参数检查
 * 2. push_back方法测试 - 验证数据添加功能
 * 3. find_nth方法测试 - 验证字符串查找功能
 * 4. substring方法测试 - 验证子字符串提取
 * 5. erase_up_to方法测试 - 验证数据删除功能
 * 6. 环形覆盖逻辑测试 - 验证当数据超过容量时的覆盖行为
 * 7. 大规模测试 - 验证在大量数据下的稳定性
 * 
 * 该测试使用自定义的TEST_ASSERT宏进行断言检查，确保各项功能符合预期。
 */

#include "../include/tools/CircularString.h"
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <iomanip> // 用于格式化输出

// 简单的断言宏，用于测试
#define TEST_ASSERT(condition, message) \
    do { \
        std::cout << "  检查: " << message << std::endl; \
        if (!(condition)) { \
            std::cerr << "  断言失败: " << message << " 在 " << __FILE__ << " 行 " << __LINE__ << std::endl; \
            return false; \
        } \
        std::cout << "  结果: 通过" << std::endl; \
    } while (0)

// 测试构造函数
bool test_constructor() {
    std::cout << "\n[构造函数测试]" << std::endl;
    
    // 测试正常构造
    std::cout << "- 测试正常构造 (容量=10)" << std::endl;
    CircularString cs(10);
    std::cout << "  创建了容量为10的环形字符串" << std::endl;
    TEST_ASSERT(cs.size() == 0, "初始大小应为0");
    TEST_ASSERT(cs.cap() == 10, "容量应为10");
    
    // 测试异常情况
    std::cout << "- 测试无效容量构造 (容量=0)" << std::endl;
    try {
        std::cout << "  尝试创建容量为0的环形字符串..." << std::endl;
        CircularString invalid(0);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::invalid_argument& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    return true;
}

// 测试push_back方法
bool test_push_back() {
    std::cout << "\n[push_back方法测试]" << std::endl;
    
    // 创建环形字符串
    std::cout << "- 创建容量为5的环形字符串" << std::endl;
    CircularString cs(5);
    
    // 测试未满时的插入
    std::cout << "- 测试插入未满情况" << std::endl;
    std::cout << "  插入字符串: \"ABC\"" << std::endl;
    cs.push_back("ABC");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 3, "大小应为3");
    TEST_ASSERT(cs.substring(0, 2) == "ABC", "内容应为ABC");
    
    // 测试满时的插入（覆盖旧元素）
    std::cout << "- 测试容量满时插入" << std::endl;
    std::cout << "  插入字符串: \"DEFG\"" << std::endl;
    cs.push_back("DEFG");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "CDEFG", "内容应为CDEFG");
    
    return true;
}

// 测试find_nth方法
bool test_find_nth() {
    std::cout << "\n[find_nth方法测试]" << std::endl;
    
    // 创建环形字符串
    std::cout << "- 创建容量为10的环形字符串并填充数据" << std::endl;
    CircularString cs(10);
    std::cout << "  插入字符串: \"ABCABC\"" << std::endl;
    cs.push_back("ABCABC");
    
    // 测试查找存在的字符
    std::cout << "- 测试查找存在的字符" << std::endl;
    std::cout << "  查找第1个'A'的位置..." << std::endl;
    size_t pos1 = cs.find_nth("A", 1);
    std::cout << "  结果: 位置=" << pos1 << std::endl;
    TEST_ASSERT(pos1 == 0, "第1个A应在位置0");
    
    std::cout << "  查找第2个'A'的位置..." << std::endl;
    size_t pos2 = cs.find_nth("A", 2);
    std::cout << "  结果: 位置=" << pos2 << std::endl;
    TEST_ASSERT(pos2 == 3, "第2个A应在位置3");
    
    // 测试查找不存在的字符
    std::cout << "- 测试查找不存在的字符" << std::endl;
    try {
        std::cout << "  查找字符'Z'..." << std::endl;
        cs.find_nth("Z", 1);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    // 测试查找超出次数的字符
    std::cout << "- 测试查找超出次数的字符" << std::endl;
    try {
        std::cout << "  查找第3个'A'..." << std::endl;
        cs.find_nth("A", 3);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    return true;
}

// 测试substring方法
bool test_substring() {
    std::cout << "\n[substring方法测试]" << std::endl;
    
    // 创建环形字符串
    std::cout << "- 创建容量为10的环形字符串并填充数据" << std::endl;
    CircularString cs(10);
    std::cout << "  插入字符串: \"ABCDEFG\"" << std::endl;
    cs.push_back("ABCDEFG");
    
    // 测试正常子串
    std::cout << "- 测试获取正常子串" << std::endl;
    std::cout << "  获取子串(1,3)..." << std::endl;
    std::string sub1 = cs.substring(1, 3);
    std::cout << "  结果: \"" << sub1 << "\"" << std::endl;
    TEST_ASSERT(sub1 == "BCD", "子串应为BCD");
    
    std::cout << "  获取子串(0,6)..." << std::endl;
    std::string sub2 = cs.substring(0, 6);
    std::cout << "  结果: \"" << sub2 << "\"" << std::endl;
    TEST_ASSERT(sub2 == "ABCDEFG", "子串应为ABCDEFG");
    
    // 测试边界情况
    std::cout << "- 测试边界情况" << std::endl;
    std::cout << "  获取子串(6,6)..." << std::endl;
    std::string sub3 = cs.substring(6, 6);
    std::cout << "  结果: \"" << sub3 << "\"" << std::endl;
    TEST_ASSERT(sub3 == "G", "子串应为G");
    
    // 测试异常情况
    std::cout << "- 测试异常情况" << std::endl;
    try {
        std::cout << "  尝试获取子串(3,2)..." << std::endl;
        cs.substring(3, 2);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    try {
        std::cout << "  尝试获取子串(0,7)..." << std::endl;
        cs.substring(0, 7);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    return true;
}

// 测试erase_up_to方法
bool test_erase_up_to() {
    std::cout << "\n[erase_up_to方法测试]" << std::endl;
    
    // 创建环形字符串
    std::cout << "- 创建容量为10的环形字符串并填充数据" << std::endl;
    CircularString cs(10);
    std::cout << "  插入字符串: \"ABCDEFG\"" << std::endl;
    cs.push_back("ABCDEFG");
    
    // 测试正常删除
    std::cout << "- 测试正常删除" << std::endl;
    std::cout << "  删除前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    std::cout << "  执行删除操作: erase_up_to(2)" << std::endl;
    cs.erase_up_to(2);
    std::cout << "  删除后内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 4, "大小应为4");
    TEST_ASSERT(cs.substring(0, 3) == "DEFG", "内容应为DEFG");
    
    // 测试删除后再插入
    std::cout << "- 测试删除后再插入" << std::endl;
    std::cout << "  插入字符串: \"HIJ\"" << std::endl;
    cs.push_back("HIJ");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 7, "大小应为7");
    TEST_ASSERT(cs.substring(0, 6) == "DEFGHIJ", "内容应为DEFGHIJ");
    
    // 测试异常情况
    std::cout << "- 测试异常情况" << std::endl;
    try {
        std::cout << "  尝试删除过多字符: erase_up_to(7)..." << std::endl;
        cs.erase_up_to(7);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
    }
    
    return true;
}

// 测试环形覆盖逻辑
bool test_circular_logic() {
    std::cout << "\n[环形覆盖逻辑测试]" << std::endl;
    
    // 创建环形字符串
    std::cout << "- 创建容量为5的环形字符串" << std::endl;
    CircularString cs(5);
    
    // 填满缓冲区
    std::cout << "- 填满缓冲区" << std::endl;
    std::cout << "  插入字符串: \"ABCDE\"" << std::endl;
    cs.push_back("ABCDE");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    
    // 添加更多字符，覆盖旧字符
    std::cout << "- 添加更多字符，覆盖旧字符" << std::endl;
    std::cout << "  插入字符串: \"FGH\"" << std::endl;
    cs.push_back("FGH");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "DEFGH", "内容应为DEFGH");
    
    // 删除部分字符
    std::cout << "- 删除部分字符" << std::endl;
    std::cout << "  执行删除操作: erase_up_to(1)" << std::endl;
    cs.erase_up_to(1);
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 3, "大小应为3");
    TEST_ASSERT(cs.substring(0, 2) == "FGH", "内容应为FGH");
    
    // 再次添加字符
    std::cout << "- 再次添加字符" << std::endl;
    std::cout << "  插入字符串: \"IJKLM\"" << std::endl;
    cs.push_back("IJKLM");
    std::cout << "  当前内容: \"" << cs.substring(0, cs.size()-1) << "\"" << std::endl;
    TEST_ASSERT(cs.size() == 5, "大小应为5");
    TEST_ASSERT(cs.substring(0, 4) == "IJKLM", "内容应为IJKLM");
    return true;
}

// 大规模测试（包含各种ASCII字符和字符串查找）
bool test_large_scale() {
    std::cout << "\n[大规模压力测试]" << std::endl;
    
    const size_t CAPACITY = 1500;
    std::cout << "- 创建容量为" << CAPACITY << "的环形字符串" << std::endl;
    CircularString cs(CAPACITY);
    
    // 生成测试数据（包含全部ASCII字符）
    std::cout << "- 生成测试数据（各种ASCII字符）" << std::endl;
    std::string test_data;
    for(int i = 1; i < 256; ++i) {  // 从1开始，避免空字符
        if (i != '\r' && i != '\n') {  // 避免单独的\r和\n干扰测试
            test_data += static_cast<char>(i);
        }
    }
    std::cout << "  生成了" << test_data.length() << "字节的测试数据" << std::endl;
    
    // 重复填充数据直到超过容量
    const int REPEAT = 10;
    std::cout << "- 重复测试数据" << REPEAT << "次" << std::endl;
    std::string big_data;
    for(int i = 0; i < REPEAT; ++i) {
        big_data += test_data + "\r\n";  // 使用实际的\r\n，而不是字符串"\\r\\n"
    }
    std::cout << "  生成了" << big_data.length() << "字节的大数据" << std::endl;
    
    // 插入大量字符（超过容量）
    std::cout << "- 插入超过容量的数据到环形字符串中" << std::endl;
    cs.push_back(big_data);
    std::cout << "  插入完成，环形字符串当前大小: " << cs.size() << std::endl;
    TEST_ASSERT(cs.size() == CAPACITY, "大小应等于容量");
    
    // 测试包含各种边界条件的查找
    std::cout << "- 测试在大数据中查找特定模式" << std::endl;
    
    // 测试查找成功的情况
    try {
        std::cout << "  查找ASCII码为64的字符('@')..." << std::endl;
        size_t pos = cs.find_nth("@", 1);
        std::cout << "  找到位置: " << pos << std::endl;
        // 这里不断言具体位置，因为很难预计，但应该能找到
    } catch (const std::out_of_range& e) {
        std::cout << "  发生意外异常: " << e.what() << std::endl;
        TEST_ASSERT(false, "应该找到字符");
    }
    
    // 测试查找失败的情况
    std::cout << "  查找一个不存在的字符串..." << std::endl;
    try {
        cs.find_nth("这是一个肯定不存在的字符串@@##$$", 1);
        TEST_ASSERT(false, "应该抛出异常");
    } catch (const std::out_of_range& e) {
        std::cout << "  捕获到预期异常: " << e.what() << std::endl;
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
        {"大规模压力测试", test_large_scale}
    };
    
    int passed = 0;
    int total = sizeof(tests) / sizeof(tests[0]);
    
    std::cout << "\n===========================================" << std::endl;
    std::cout << "开始运行 " << total << " 个测试用例" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    for (const auto& test : tests) {
        std::cout << "\n[" << (passed+1) << "/" << total << "] 运行测试: " << test.name;
        
        bool result = test.test_func();
        
        if (result) {
            std::cout << " [通过]" << std::endl;
            passed++;
        } else {
            std::cout << " [失败]" << std::endl;
        }
    }
    
    std::cout << "\n===========================================" << std::endl;
    std::cout << "测试完成! 结果: " << passed << "/" << total << " 通过 ";
    
    if (passed == total) {
        std::cout << "(" << std::setprecision(2) << std::fixed << 100.0 << "%)";
    } else {
        std::cout << "(" << std::setprecision(2) << std::fixed 
                 << (static_cast<double>(passed) / total * 100.0) << "%)";
    }
    std::cout << std::endl;
    std::cout << "===========================================" << std::endl;
}

int main() {
    std::cout << "\n==============================================" << std::endl;
    std::cout << "      CircularString 类测试程序" << std::endl;
    std::cout << "      测试各种环形字符串操作的正确性" << std::endl;
    std::cout << "==============================================" << std::endl;
    
    run_all_tests();
    return 0;
}