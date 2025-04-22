#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include "../include/base64.h"
#include "../include/AhoCorasick.h"

// 读取utf-8格式文件内容并转换为字符串
std::string readFileToUtf8(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        std::cerr << "打开文件失败 " << filepath << std::endl;
        return "";
    }
    std::stringstream ss;
    ss<<file.rdbuf();
    file.close();
    return ss.str();
}

int main(int argc, char* argv[]) {
    // 默认输入文件路径
    std::string inputFilePath = "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/auto_AC/file/input.txt";
    
    // 如果提供了命令行参数，使用指定的文件路径
    if (argc > 1) {
        inputFilePath = argv[1];
        std::cout << "使用指定的输入文件: " << inputFilePath << std::endl;
    }

    // 1. 初始化AC
    AhoCorasick ac;

    // 2. 读取敏感词库并构建Trie树
    // std::ifstream dictFile("F:/c++/.vscode/auto_AC/file/sensitive.txt");
    std::ifstream dictFile("/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/auto_AC/file/sensitive.txt");
    if (!dictFile.is_open()) {
        std::cerr << "打开文件失败" << std::endl;
        return 1;
    }
    std::string line;
    // while (std::getline(dictFile, line)) {
    //     if (!line.empty()) {
    //         //std::cout << line << std::endl;
    //         ac.buildTrie(line);
    //     }
    // }
    // 修改敏感词读取逻辑，去除CR字符
    while (std::getline(dictFile, line)) {
        if (!line.empty()) {
            // 处理CRLF换行符
            if (line.back() == '\r') {
                line.pop_back();
            }
            
            ac.buildTrie(line);
        }
    }
    dictFile.close();
    std::cout << "Trie树构建完成" << std::endl;
    ac.buildFailPointer();
    std::cout<<"失败指针构建完成"<<std::endl;


    // 3. 读取邮件内容
    std::string content = readFileToUtf8(inputFilePath);
    // std::cout<<"内容"<<content<<std::endl;
    if (content.empty()) {
        return 1;
    }
    // Base64 base64;
    //std::string content = "woshi你好哈哈来你好我是大强asdkfjalsdfjlajdf安科技发达卢卡斯的积分卡萨丁积分卡地方";

    // 4. 执行敏感词查询
    auto start = std::chrono::high_resolution_clock::now();
    ac.queryWord(content);
    //std::cout<<content;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "消耗" << duration.count() << " 毫秒" << std::endl;

    return 0;
}