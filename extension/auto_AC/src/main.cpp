#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include "../include/base64.h"
#include "../include/AhoCorasick.h"
#include "../../../include/config/config_parser.h"

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

// 判断文件是否存在
bool fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

int main(int argc, char* argv[]) {
    // 寻找配置文件的可能路径
    std::string configPath = "config.ini";  // 当前目录
    
    // 可能的配置文件路径列表
    std::vector<std::string> possibleConfigPaths = {
        "config.ini",                 // 当前目录
        "../config.ini",              // 上级目录
        "../../config.ini",           // 上上级目录
        "../../../config.ini",        // 上上上级目录
        "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/config.ini"  // 完整路径
    };
    
    // 默认词典路径
    std::string defaultDictPath = "extension/auto_AC/file/dictionary.txt";
    // 默认敏感词典文件路径（绝对路径，确保可以找到）
    std::string absoluteDictPath = "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/auto_AC/file/sensitive.txt";
    
    // 寻找配置文件
    flow_table::ConfigParser config;
    bool configLoaded = false;
    
    for (const auto& path : possibleConfigPaths) {
        if (fileExists(path)) {
            configLoaded = config.loadFromFile(path);
            if (configLoaded) {
                std::cout << "成功加载配置文件: " << path << std::endl;
                configPath = path;
                break;
            }
        }
    }
    
    if (!configLoaded) {
        std::cerr << "警告: 无法找到或加载配置文件，将使用默认值" << std::endl;
    }
    
    // 默认输入文件路径 - 直接使用命令行参数
    std::string inputFilePath;
    
    // 如果提供了命令行参数，使用指定的文件路径
    if (argc > 1) {
        inputFilePath = argv[1];
        std::cout << "使用指定的输入文件: " << inputFilePath << std::endl;
    } else {
        inputFilePath = config.getString("Paths.test_input", 
            "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/auto_AC/file/input.txt");
        std::cout << "使用配置的输入文件: " << inputFilePath << std::endl;
    }
    
    // 获取字典文件路径 - 尝试从配置获取，失败则使用默认绝对路径
    std::string dictionaryPath = config.getString("Paths.keyword_dict", absoluteDictPath);
    
    // 验证文件存在性
    if (!fileExists(dictionaryPath)) {
        std::cerr << "词典文件不存在: " << dictionaryPath << std::endl;
        std::cerr << "尝试使用默认词典: " << absoluteDictPath << std::endl;
        dictionaryPath = absoluteDictPath;
        
        if (!fileExists(dictionaryPath)) {
            std::cerr << "默认词典文件也不存在，无法继续" << std::endl;
            return 1;
        }
    }
    
    std::cout << "使用词典文件: " << dictionaryPath << std::endl;

    // 1. 初始化AC
    AhoCorasick ac;

    // 2. 读取敏感词库并构建Trie树
    std::ifstream dictFile(dictionaryPath);
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
    std::cout<<"邮件内容: "<<content<<std::endl;
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
    // std::cout << "消耗" << duration.count() << " 毫秒" << std::endl;

    return 0;
}