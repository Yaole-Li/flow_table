/**
 * @file test_email_keyword_detection.cpp
 * @brief 邮件内容解析和关键词检测集成测试
 * 
 * 本测试文件用于验证邮件内容解析和关键词检测的完整工作流程。
 * 主要功能：
 * 1. 运行邮件解析程序(test_s2c_parser)，从IMAP协议响应中提取邮件内容并保存到文件
 * 2. 运行关键词检测程序(keyword_detector)，使用AC自动机算法在邮件内容中检测敏感词
 * 3. 验证整个处理流程的完整性和正确性
 * 
 * 使用配置文件(config.ini)获取以下参数：
 * - 解析后的邮件内容保存路径
 * - 敏感词典文件路径
 * - 测试用长文本输入文件路径
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include "../include/config/config_parser.h"

// 获取项目根目录的工具函数
std::string getProjectRoot() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string currentPath(cwd);
        // 查找flow_table目录
        size_t pos = currentPath.find("flow_table");
        if (pos != std::string::npos) {
            // 返回到flow_table目录的路径
            return currentPath.substr(0, pos + 10); // 10是"flow_table"的长度
        }
    }
    // 返回默认路径
    std::cerr << "警告: 无法确定项目根目录，使用相对路径" << std::endl;
    return "./";
}

int main() {
    std::cout << "===== 开始邮件内容解析和关键词检测集成测试 =====" << std::endl;
    
    // 获取项目根目录
    std::string projectRoot = getProjectRoot();
    
    // 从配置文件读取参数
    flow_table::ConfigParser config;
    if (!config.loadFromFile("config.ini")) {
        std::cerr << "警告: 无法加载配置文件 config.ini，将使用默认值" << std::endl;
    }
    
    // 邮件内容文件路径
    std::string emailContentPath = config.getString("Paths.test_email_content", "test/parsed_email_content.txt");
    emailContentPath = projectRoot + "/" + emailContentPath;
    
    // 1. 运行邮件解析程序，解析邮件并保存内容到文件
    std::cout << "\n第一步：运行邮件解析程序..." << std::endl;
    
    // 使用项目根目录构建命令
    int parseResult = system(("cd " + projectRoot + "/build && ./test_s2c_parser").c_str());
    
    if (parseResult != 0) {
        std::cerr << "错误: 邮件解析程序运行失败，错误码: " << parseResult << std::endl;
        return 1;
    }
    
    std::cout << "邮件解析程序运行成功\n" << std::endl;
    
    // 检查邮件内容文件是否存在
    if (access(emailContentPath.c_str(), F_OK) != 0) {
        std::cerr << "邮件内容文件不存在，可能解析失败！" << std::endl;
        return 1;
    }
    
    std::cout << "\n第一步完成：邮件内容已保存到 " << emailContentPath << std::endl;
    
    // 2. 运行关键词检测程序，检测敏感词
    std::cout << "第二步：运行关键词检测程序...\n" << std::endl;
    std::cout << "解析后的邮件内容文件: " << emailContentPath << std::endl;
    
    // 使用项目根目录构建命令
    std::string cmd = "cd " + projectRoot + "/build && ./keyword_detector " + emailContentPath;
    std::cout << "执行命令: " << cmd << std::endl;
    
    int detectResult = system(cmd.c_str());
    
    if (detectResult != 0) {
        std::cerr << "错误: 关键词检测程序运行失败，错误码: " << detectResult << std::endl;
        return 1;
    }
    
    std::cout << "\n===== 邮件内容解析和关键词检测集成测试完成 =====" << std::endl;
    return 0;
}
