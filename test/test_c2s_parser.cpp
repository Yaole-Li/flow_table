/**
 * @file test_c2s_parser.cpp
 * @brief IMAP协议客户端到服务器(C2S)解析器测试
 * 
 * 本测试文件用于验证IMAP协议客户端命令的解析功能。
 * 主要功能：
 * 1. 测试IMAP客户端命令的解析能力
 * 2. 包括以下命令的解析测试：
 *    - LOGIN命令（登录）
 *    - SELECT命令（选择邮箱）
 *    - FETCH命令（获取邮件）
 *    - LOGOUT命令（登出）
 * 3. 使用extension/auto_AC/file/c2s数据.txt中的实际命令进行测试
 * 
 * 使用配置文件(config.ini)获取参数
 * 
 * 本测试是IMAP邮件解析系统的基础部分，负责解析客户端发送的IMAP命令。
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>  // 用于文件操作
#include <sstream>  // 用于字符串流
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>  // 用于getcwd函数
#include "../include/flows/flow_manager.h"
#include "../include/tools/types.h"
#include "../include/config/config_parser.h"  // 使用项目的配置解析器

using namespace flow_table;

// 获取项目根目录的函数
std::string getProjectRoot() {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        std::string currentPath(buffer);
        size_t pos = currentPath.find("flow_table");
        if (pos != std::string::npos) {
            return currentPath.substr(0, pos + 10); // 10是"flow_table"的长度
        }
    }
    return ""; // 如果找不到flow_table目录，返回空字符串
}

int main() {
    std::cout << "======= IMAP 客户端命令解析测试 =======" << std::endl;
    
    // 创建单个HashFlowTable和Flow对象
    HashFlowTable flowTable;
    
    // 创建四元组
    FourTuple fourTuple;
    fourTuple.srcIPvN = 4;  // 使用IPv4
    fourTuple.dstIPvN = 4;  // 使用IPv4
    fourTuple.srcIPv4 = inet_addr("192.168.1.100");
    fourTuple.dstIPv4 = inet_addr("192.168.1.200");
    fourTuple.sourcePort = htons(12345);
    fourTuple.destPort = htons(143); // IMAP标准端口
    
    // 模拟完整IMAP会话的命令序列
    std::vector<std::pair<std::string, std::string>> commands = {
        {"a1 login 1094825151@qq.com mlqecbulvgjjxhdf", "LOGIN命令"},
        {"a2 SELECT \"INBOX\"", "SELECT命令"},
        {"a3 FETCH 1:10 (UID FLAGS BODY.PEEK[HEADER.FIELDS (FROM SUBJECT DATE)])", "FETCH命令 - 邮件头"},
        {"a4 fetch 6 body.peek[]", "FETCH命令 - 完整邮件"},
        {"a5 fetch 7 body.peek[]", "FETCH命令 - 另一封邮件"},
        // {"a6 logout", "LOGOUT命令"}
    };
    
    // 逐个处理命令并保持同一个流
    for (const auto& cmd : commands) {
        std::cout << "\n===== 处理: " << cmd.second << " =====" << std::endl;
        std::cout << "命令: " << cmd.first << std::endl;
        
        // 创建输入包
        InputPacket packet;
        packet.type = "C2S";
        packet.payload = cmd.first + "\r\n"; // 添加CRLF结束符
        packet.fourTuple = fourTuple;
        
        // 处理数据包
        flowTable.processPacket(packet);
    }
    
    // 获取Flow对象并输出完整的消息历史
    Flow* flow = flowTable.getOrCreateFlow(fourTuple);
    if (flow) {
        std::cout << "\n===== 完整IMAP会话解析结果 =====" << std::endl;
        flow->outputMessages();
    } else {
        std::cerr << "错误：未能获取Flow对象" << std::endl;
    }
    
    // 从文件加载测试 - 可选部分，如果需要从c2s数据.txt文件读取可以取消注释
    /*
    std::cout << "\n===== 从文件加载C2S命令测试 =====" << std::endl;
    
    HashFlowTable batchFlowTable;
    FourTuple batchFourTuple;
    batchFourTuple.srcIPvN = 4;  // 使用IPv4
    batchFourTuple.dstIPvN = 4;  // 使用IPv4
    batchFourTuple.srcIPv4 = inet_addr("192.168.1.100");
    batchFourTuple.dstIPv4 = inet_addr("192.168.1.200");
    batchFourTuple.sourcePort = htons(54321);
    batchFourTuple.destPort = htons(143);
    
    // 获取C2S数据文件路径
    std::string projectRoot = getProjectRoot();
    std::string c2sDataPath = projectRoot + "/extension/auto_AC/file/c2s数据.txt";
    
    std::ifstream c2sFile(c2sDataPath);
    if (!c2sFile.is_open()) {
        std::cerr << "错误：无法打开C2S数据文件: " << c2sDataPath << std::endl;
        return 1;
    }
    
    std::string line;
    int commandCount = 0;
    while (std::getline(c2sFile, line)) {
        if (line.empty()) continue;
        
        // 创建输入包
        InputPacket batchPacket;
        batchPacket.type = "C2S";
        batchPacket.payload = line + "\r\n"; // 添加CRLF结束符
        batchPacket.fourTuple = batchFourTuple;
        
        // 处理数据包
        batchFlowTable.processPacket(batchPacket);
        commandCount++;
        
        std::cout << "已处理命令 #" << commandCount << ": " << line << std::endl;
    }
    c2sFile.close();
    
    // 输出批量解析结果
    Flow* batchFlow = batchFlowTable.getOrCreateFlow(batchFourTuple);
    if (batchFlow) {
        std::cout << "\n===== 批量命令解析结果 =====" << std::endl;
        batchFlow->outputMessages();
    } else {
        std::cerr << "错误：未能获取批量Flow对象" << std::endl;
    }
    */
    
    std::cout << "\n======= 测试完成 =======" << std::endl;
    return 0;
}
