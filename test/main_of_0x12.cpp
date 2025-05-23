#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"
#include "../include/config/config_parser.h"
#include "../extension/auto_AC/include/AhoCorasick.h"

// 通信实体结构体
typedef struct {
    char Role;          // 角　　色 C/S
    unsigned char IPvN; // 版　　本 4/6

    union {
        unsigned int IPv4;      // IPv4地址
        unsigned char IPv6[16]; // IPv6地址
    };

    unsigned short Port; // 端　　口
} ENTITY;

// 交互任务结构体
typedef struct {
    // 指令区
    union {
        unsigned char Inform; // 通告指令 0x12=数据传输 0x13=关闭
        unsigned char Action; // 动作指令 0x22=转发 0x21=知晓
    };

    unsigned char Option; // 标志选项

    // 标识区
    unsigned short Thread; // 线程编号
    unsigned short Number; // 链路标识

    // 通信区
    ENTITY Source; // 源　　端
    ENTITY Target; // 宿　　端

    // 承载区
    unsigned char* Buffer; // 数　　据
    unsigned int Length;   // 长　　度
    unsigned int Volume;   // 容　　限
} TASK;

// 全局变量
static flow_table::HashFlowTable* flowTable = nullptr;
// 关键词检测相关全局变量
static AhoCorasick* acDetector = nullptr;
static flow_table::ConfigParser* configPtr = nullptr;
static std::string projectRoot;

// ------------------------------ 1. 全局初始化 ------------------------------
// 该部分只在程序启动时执行一次

// 获取当前目录的工具函数
std::string getCurrentDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return std::string(cwd) + "/";
    }
    // 返回默认路径
    std::cerr << "警告: 无法获取当前目录，使用相对路径" << std::endl;
    return "./";
}

// 获取项目根目录的工具函数 - 保留以向后兼容
std::string getProjectRoot() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string currentPath(cwd);
        // 查找flow_table目录
        size_t pos = currentPath.find("flow_table");
        if (pos != std::string::npos) {
            // 返回到flow_table目录的路径
            return currentPath.substr(0, pos + 10) + "/"; // 10是"flow_table"的长度
        }
    }
    // 返回默认路径
    std::cerr << "警告: 无法确定项目根目录，使用相对路径" << std::endl;
    return "./";
}

// 判断文件是否存在
bool fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

// 初始化关键词检测器
bool initKeywordDetector() {
    std::cout << "初始化关键词检测器..." << std::endl;
    
    // 从配置文件获取敏感词典路径
    std::string dictionaryPath = configPtr->getString("Paths.keyword_dict", 
        projectRoot + "/extension/auto_AC/file/sensitive.txt");
    
    // 验证文件存在性
    if (!fileExists(dictionaryPath)) {
        std::cerr << "词典文件不存在: " << dictionaryPath << std::endl;
        return false;
    }
    
    std::cout << "使用词典文件: " << dictionaryPath << std::endl;
    
    // 创建AC自动机
    acDetector = new AhoCorasick();
    
    // 读取敏感词库并构建Trie树
    std::ifstream dictFile(dictionaryPath);
    if (!dictFile.is_open()) {
        std::cerr << "打开词典文件失败" << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(dictFile, line)) {
        if (!line.empty()) {
            // 处理CRLF换行符
            if (line.back() == '\r') {
                line.pop_back();
            }
            acDetector->buildTrie(line);
        }
    }
    dictFile.close();
    
    std::cout << "Trie树构建完成" << std::endl;
    acDetector->buildFailPointer();
    std::cout << "失败指针构建完成" << std::endl;
    
    return true;
}

// 全局配置文件路径
std::string configFilePath;

void GlobalInit() {
    std::cout << "执行全局初始化..." << std::endl;
    
    // 获取项目根目录
    projectRoot = getProjectRoot();
    
    // 初始化配置解析器
    configPtr = new flow_table::ConfigParser();
    
    // 尝试加载配置文件，按以下顺序尝试：
    // 1. 使用命令行指定的配置文件路径（如果有）
    // 2. 尝试当前目录下的config.ini
    // 3. 尝试项目根目录下的config.ini
    bool configLoaded = false;
    
    if (!configFilePath.empty()) {
        // 使用命令行指定的配置文件
        std::cout << "尝试加载命令行指定的配置文件: " << configFilePath << std::endl;
        configLoaded = configPtr->loadFromFile(configFilePath);
    }
    
    if (!configLoaded) {
        // 尝试当前目录
        std::string currentDirConfig = getCurrentDir() + "config.ini";
        std::cout << "尝试加载当前目录下的配置文件: " << currentDirConfig << std::endl;
        configLoaded = configPtr->loadFromFile(currentDirConfig);
    }
    
    if (!configLoaded) {
        // 尝试项目根目录
        std::string projectRootConfig = projectRoot + "config.ini";
        std::cout << "尝试加载项目根目录下的配置文件: " << projectRootConfig << std::endl;
        configLoaded = configPtr->loadFromFile(projectRootConfig);
    }
    
    if (!configLoaded) {
        std::cerr << "警告: 无法加载任何配置文件，将使用默认值" << std::endl;
    } else {
        std::cout << "成功加载配置文件" << std::endl;
    }
    
    // 初始化关键词检测器
    if (!initKeywordDetector()) {
        std::cerr << "警告: 关键词检测器初始化失败" << std::endl;
    }
    
    std::cout << "全局初始化完成" << std::endl;
}

// ------------------------------ 2. 单线程初始化 ------------------------------
// 该部分在每个工作线程启动时执行
void ThreadInit() {
    std::cout << "执行线程初始化..." << std::endl;
    
    // 创建哈希流表（每个线程一个实例）
    flowTable = new flow_table::HashFlowTable();
    
    // 从配置文件中读取流超时时间（配置文件中的Flow.flow_timeout参数）
    // 默认值为120000毫秒（120秒）
    int64_t flowTimeoutMs = 120000; // 默认值
    
    if (configPtr) {
        flowTimeoutMs = configPtr->getInt64("Flow.flow_timeout", 120000);
        std::cout << "从配置文件读取流超时时间: " << flowTimeoutMs << " 毫秒" << std::endl;
    } else {
        std::cout << "使用默认流超时时间: 120000 毫秒" << std::endl;
    }
    
    // 设置流超时时间
    flowTable->setFlowTimeout(flowTimeoutMs);
    
    std::cout << "线程初始化完成" << std::endl;
}

// ------------------------------ 3. Filter 处理函数 ------------------------------
// 数据过滤函数，处理每个数据包

// 执行关键词检测
void performKeywordDetection(const std::string& content) {
    if (!acDetector) {
        std::cerr << "错误: 关键词检测器未初始化" << std::endl;
        return;
    }
    
    std::cout << "\n===== 执行关键词检测 =====" << std::endl;
    std::cout << "内容长度: " << content.length() << " 字节" << std::endl;
    
    // 执行检测
    acDetector->queryWord(content);
}

int Filter(TASK *Import, TASK **Export) {
    // 置动作为通告
    *Export = Import;

    switch(Import->Inform) {
        case 0X12: {
            // 设置动作为转发
            (*Export)->Action = 0X22;

            // 2. 根据 TASK 中的源端和宿端创建四元组
            FourTuple fourTuple;
            
            // 判断数据包方向
            bool isC2S = (Import->Source.Role == 'C');
            
            // 设置源IP和端口 - 始终保持C2S方向
            if (isC2S) {
                // C2S方向，正常设置
                // 设置源IP（客户端）
                if (Import->Source.IPvN == 4) {
                    fourTuple.srcIPvN = 4;
                    fourTuple.srcIPv4 = Import->Source.IPv4;
                } else if (Import->Source.IPvN == 6) {
                    fourTuple.srcIPvN = 6;
                    memcpy(fourTuple.srcIPv6, Import->Source.IPv6, 16);
                }
                fourTuple.sourcePort = Import->Source.Port;
                
                // 设置目标IP（服务器）
                if (Import->Target.IPvN == 4) {
                    fourTuple.dstIPvN = 4;
                    fourTuple.dstIPv4 = Import->Target.IPv4;
                } else if (Import->Target.IPvN == 6) {
                    fourTuple.dstIPvN = 6;
                    memcpy(fourTuple.dstIPv6, Import->Target.IPv6, 16);
                }
                fourTuple.destPort = Import->Target.Port;
            } else {
                // S2C方向，反转方向使其成为C2S方向
                // 设置源IP（客户端，即Target）
                if (Import->Target.IPvN == 4) {
                    fourTuple.srcIPvN = 4;
                    fourTuple.srcIPv4 = Import->Target.IPv4;
                } else if (Import->Target.IPvN == 6) {
                    fourTuple.srcIPvN = 6;
                    memcpy(fourTuple.srcIPv6, Import->Target.IPv6, 16);
                }
                fourTuple.sourcePort = Import->Target.Port;
                
                // 设置目标IP（服务器，即Source）
                if (Import->Source.IPvN == 4) {
                    fourTuple.dstIPvN = 4;
                    fourTuple.dstIPv4 = Import->Source.IPv4;
                } else if (Import->Source.IPvN == 6) {
                    fourTuple.dstIPvN = 6;
                    memcpy(fourTuple.dstIPv6, Import->Source.IPv6, 16);
                }
                fourTuple.destPort = Import->Source.Port;
            }
            
            // 3. 创建InputPacket，只包含数据、类型和四元组
            flow_table::InputPacket packet;
            
            // 设置数据包的有效载荷
            std::string payload(reinterpret_cast<char*>(Import->Buffer), Import->Length);
            packet.payload = payload;
            
            // 设置数据包类型（即源角色）
            packet.type = isC2S ? "C2S" : "S2C";
            
            // 设置四元组 - 现在四元组始终是C2S方向
            packet.fourTuple = fourTuple;
            
            // 4. 处理数据包
            bool processed = flowTable->processPacket(packet);
            
            // 5. 输出处理结果
            if (processed) {
                // 输出处理结果
                flowTable->outputResults();
                
                // 如果是S2C数据包，执行S2C解析处理后可能会产生邮件内容需要检测
                if (!isC2S) {
                    // 获取所有流，并从中寻找包含FETCH响应的邮件
                    std::vector<flow_table::Flow*> allFlows = flowTable->getAllFlows();
                    
                    for (const auto& flow : allFlows) {
                        // 每个流处理后会输出消息，这里不需要直接获取S2C消息
                        // 邮件内容在解析时已经输出，我们可以扫描所有流中的邮件
                        
                        // 从已解析的数据中寻找邮件内容 - 这里实际上需要从flow获取S2C消息
                        // 但由于Flow类没有提供公共的获取S2C消息的方法，我们只能通过输出结果检测
                        std::cout << "检测到S2C数据包，邮件内容已处理" << std::endl;
                        
                        // TODO: 如果要完全访问流中的S2C消息和邮件内容，需要向Flow类添加获取S2C消息的公共方法
                        // 例如：const std::vector<Message>& getS2CMessages() const;
                        // 目前，我们只能检测包含了email关键词的文本内容
                        
                        // 直接对输入的S2C数据包内容进行检测
                        std::string contentToCheck = std::string(reinterpret_cast<char*>(Import->Buffer), Import->Length);
                        if (!contentToCheck.empty()) {
                            std::cout << "对S2C数据包内容进行关键词检测..." << std::endl;
                            performKeywordDetection(contentToCheck);
                        }
                        
                        // 只处理当前输入数据，跳出循环
                        break;
                    }
                }
                
            }

            break;
        }
        default:
            (*Export)->Action = 0X21;
            break;
    }

    return 0;
}

// ------------------------------ 4. Remove 清理函数 ------------------------------
// 负责资源释放和清理
void Remove() {
    std::cout << "执行清理..." << std::endl;
    
    // 清理所有资源
    if (flowTable != nullptr) {
        // 获取所有流对象并输出最终结果
        flowTable->outputResults();
        
        // 删除哈希流表
        // 用析构函数
        delete flowTable;
        flowTable = nullptr;
    }
    
    // 清理关键词检测器
    if (acDetector != nullptr) {
        delete acDetector;
        acDetector = nullptr;
    }
    
    // 清理配置解析器
    if (configPtr != nullptr) {
        delete configPtr;
        configPtr = nullptr;
    }
    
    std::cout << "清理完成" << std::endl;
}

// 测试主函数，模拟插件的使用
int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc > 1) {
        // 第一个参数作为配置文件路径
        configFilePath = argv[1];
        std::cout << "使用命令行指定的配置文件: " << configFilePath << std::endl;
    }
    /*
    修改了以下问题: 
    1.不做线程，每次包过来的时候都看下最旧的流是否超时。 
    2.logout的时候直接清理 
    3.解决哈希冲突 
    4.四元组反向 
    5.输出Message后判断超时，重点是建立一个虚拟链表，按照时间排序(我用了时间桶算法)
    */

    // 1. 全局初始化（只执行一次）
    GlobalInit();
    
    // 2. 线程初始化（每个线程执行一次）
    ThreadInit();
    
    std::cout << "\n===== 测试 C2S 方向数据包 =====" << std::endl;
    {
        // 3.1 创建 C2S 方向的测试数据包
        TASK task;
        task.Inform = 0x12;
        task.Source.Role = 'C';  // 客户端角色
        task.Source.IPvN = 4;
        task.Source.IPv4 = 0x01020304; // 1.2.3.4
        task.Source.Port = 1234;
        task.Target.Role = 'S';  // 服务器角色
        task.Target.IPvN = 4;
        task.Target.IPv4 = 0x05060708; // 5.6.7.8
        task.Target.Port = 143;  // IMAP标准端口
        
        // 创建 C2S 测试数据 - IMAP客户端命令
        const char* c2sData = "a0004 FETCH 1:13 (FLAGS INTERNALDATE RFC822.SIZE BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT TO CC MESSAGE-ID REFERENCES CONTENT-TYPE IN-REPLY-TO REPLY-TO)])\r\n";
        size_t dataLen = strlen(c2sData);
        unsigned char* buffer = new unsigned char[dataLen];
        memcpy(buffer, c2sData, dataLen);
        
        task.Buffer = buffer;
        task.Length = dataLen;
        
        // 处理 C2S 数据包
        TASK* exportTask = nullptr;
        std::cout << "处理C2S数据包: " << c2sData << std::endl;
        Filter(&task, &exportTask);
        
        delete[] buffer;
    }
    
    std::cout << "\n===== 测试 S2C 方向数据包 =====" << std::endl;
    {
        // 3.2 创建 S2C 方向的测试数据包
        TASK task;
        task.Inform = 0x12;
        task.Source.Role = 'S';  // 服务器角色
        task.Source.IPvN = 4;
        task.Source.IPv4 = 0x05060708; // 5.6.7.8
        task.Source.Port = 143;  // IMAP标准端口
        task.Target.Role = 'C';  // 客户端角色
        task.Target.IPvN = 4;
        task.Target.IPv4 = 0x01020304; // 1.2.3.4
        task.Target.Port = 1234;
        
        // 直接使用test_s2c_parser.cpp中验证过的S2C响应数据
        const char* s2cData = "* 1 FETCH (UID 26 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {114}\r\n"
                              "From: =?utf-8?B?5aeTIOWQjQ==?= <z1459384884@outlook.com>\r\n"
                              "Subject: 11111\r\n"
                              "Date: Tue, 8 Apr 2025 12:53:48 +0000\r\n"
                              "\r\n"
                              ")\r\n";
        size_t dataLen = strlen(s2cData);
        unsigned char* buffer = new unsigned char[dataLen];
        memcpy(buffer, s2cData, dataLen);
        
        task.Buffer = buffer;
        task.Length = dataLen;
        
        // 处理 S2C 数据包
        TASK* exportTask = nullptr;
        std::cout << "处理S2C数据包: " << s2cData << std::endl;
        Filter(&task, &exportTask);
        
        delete[] buffer;
    }
    
    // 4. 清理资源（程序结束时执行）
    Remove();
    
    /*
    TODO：
    把拿不住的数据软编码，放在配置文件中
    集成 s2c 功能
    openssl 库
    */

    return 0;
}