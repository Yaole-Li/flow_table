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
        return 0;
    }
    std::stringstream ss;
    ss<<file.rdbuf();
    file.close();
    return ss.str();
}

int main() {
    // 1. 初始化AC
    AhoCorasick ac;

    // 2. 读取敏感词库并构建Trie树
    std::ifstream dictFile("F:/c++/.vscode/auto_AC/file/sensitive.txt");
    if (!dictFile) {
        std::cerr << "打开文件失败" << std::endl;
        return 1;
    }
    std::string line;
    while (std::getline(dictFile, line)) {
        if (!line.empty()) {
            ac.buildTrie(line);
        }
    }
    dictFile.close();
    std::cout << "Trie树构建完成" << std::endl;
    ac.buildFailPointer();
    std::cout<<"失败指针构建完成"<<std::endl;
    dictFile.close();

    // 3. 读取邮件内容
    std::string content = readFileToUtf8("F:/c++/.vscode/auto_AC/file/input.txt");
    if (content.empty()) {
        return 1;
    }
    // Base64 base64;
    // std::string content = base64.Decode(encodedContent.c_str(), encodedContent.size());

    // 4. 执行敏感词查询
    auto start = std::chrono::high_resolution_clock::now();
    ac.queryWord(content);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "uuu " << duration.count() << " 毫秒" << std::endl;

    return 0;
}