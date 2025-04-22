#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main() {
    std::cout << "===== 开始邮件内容解析和关键词检测集成测试 =====" << std::endl;
    
    // 邮件内容文件路径
    std::string emailContentPath = "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/test/parsed_email_content.txt";
    
    // 1. 运行邮件解析程序，解析邮件并保存内容到文件
    std::cout << "\n第一步：运行邮件解析程序..." << std::endl;
    int parseResult = system("cd /Users/liyaole/Documents/works/c_work/imap_works/flow_table/build && ./test_s2c_parser");
    
    if (parseResult != 0) {
        std::cerr << "邮件解析程序执行失败！" << std::endl;
        return 1;
    }
    
    // 检查邮件内容文件是否存在
    if (access(emailContentPath.c_str(), F_OK) != 0) {
        std::cerr << "邮件内容文件不存在，可能解析失败！" << std::endl;
        return 1;
    }
    
    std::cout << "\n第一步完成：邮件内容已保存到 " << emailContentPath << std::endl;
    
    // 2. 运行关键词检测程序，检测邮件内容中的敏感词
    std::cout << "\n第二步：运行关键词检测程序..." << std::endl;
    std::string cmd = "cd /Users/liyaole/Documents/works/c_work/imap_works/flow_table/build && ./keyword_detector " + emailContentPath;
    int detectResult = system(cmd.c_str());
    
    if (detectResult != 0) {
        std::cerr << "关键词检测程序执行失败！" << std::endl;
        return 1;
    }
    
    std::cout << "\n===== 邮件内容解析和关键词检测集成测试完成 =====" << std::endl;
    
    return 0;
}
