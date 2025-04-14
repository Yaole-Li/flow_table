#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"

// 创建一个InputPacket结构体，模拟IMAP命令数据包
flow_table::InputPacket createImapPacket(const std::string& payload, const std::string& type, 
                                         const std::string& srcIP, int srcPort, 
                                         const std::string& dstIP, int dstPort) {
    flow_table::InputPacket packet;
    packet.payload = payload;
    packet.type = type;
    
    // 设置四元组
    FourTuple& fourTuple = packet.fourTuple;
    
    // 设置源IP
    fourTuple.srcIPvN = 4;  // IPv4
    fourTuple.srcIPv4 = inet_addr(srcIP.c_str());
    fourTuple.sourcePort = srcPort;
    
    // 设置目标IP
    fourTuple.dstIPvN = 4;  // IPv4
    fourTuple.dstIPv4 = inet_addr(dstIP.c_str());
    fourTuple.destPort = dstPort;
    
    return packet;
}

// 生成一个随机IP地址字符串
std::string generateRandomIP() {
    // 简单随机IP生成，实际应用中可以使用更复杂的随机数生成
    int a = 10 + (rand() % 245);
    int b = 1 + (rand() % 254);
    int c = 1 + (rand() % 254);
    int d = 2 + (rand() % 253);
    
    return std::to_string(a) + "." + std::to_string(b) + "." + 
           std::to_string(c) + "." + std::to_string(d);
}

// 生成一个随机端口号
int generateRandomPort() {
    // 生成1024-65535之间的随机端口
    return 1024 + (rand() % 64511);
}

// 打印四元组信息
void printFourTuple(const FourTuple& tuple) {
    std::cout << "四元组信息: ";
    if (tuple.srcIPvN == 4) {
        struct in_addr addr;
        addr.s_addr = htonl(tuple.srcIPv4);
        std::cout << inet_ntoa(addr) << ":" << tuple.sourcePort << " -> ";
        
        addr.s_addr = htonl(tuple.dstIPv4);
        std::cout << inet_ntoa(addr) << ":" << tuple.destPort;
    } else {
        std::cout << "[IPv6地址]:" << tuple.sourcePort << " -> [IPv6地址]:" << tuple.destPort;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "===== IMAP解析复杂测试 =====" << std::endl;
    
    // 设置随机种子
    srand(time(nullptr));
    
    // 创建一个作用域，使flowTable在作用域结束时自动销毁
    {
        // 1. 创建哈希流表
        flow_table::HashFlowTable flowTable;
        flowTable.setFlowTimeout(120000); // 设置超时时间为120秒
        
        // 2. 创建多个不同的流和复杂的IMAP命令
        
        // 流1: 标准IMAP会话
        std::string clientIP1 = "192.168.1.100";
        std::string serverIP1 = "10.0.0.1";
        int clientPort1 = 12345;
        int serverPort1 = 143;
        
        // 流2: 另一个IMAP会话
        std::string clientIP2 = "192.168.1.101";
        std::string serverIP2 = "10.0.0.1";
        int clientPort2 = 54321;
        int serverPort2 = 143;
        
        // 流3: 随机生成的流
        std::string clientIP3 = generateRandomIP();
        std::string serverIP3 = "10.0.0.2";
        int clientPort3 = generateRandomPort();
        int serverPort3 = 143;
        
        // 创建一系列复杂的IMAP命令
        std::vector<std::pair<int, std::string>> commands = {
            // 流ID, 命令内容
            {1, "A001 LOGIN user password\r\n"},
            {1, "A002 SELECT INBOX\r\n"},
            {2, "B001 LOGIN admin secure_password\r\n"},
            {1, "A003 FETCH 1:5 (FLAGS BODY[HEADER])\r\n"},
            {3, "C001 LOGIN test test123\r\n"},
            {2, "B002 LIST \"\" *\r\n"},
            {1, "A004 FETCH 1 (BODY[TEXT])\r\n"},
            {3, "C002 SELECT \"Sent Items\"\r\n"},
            {2, "B003 STATUS INBOX (MESSAGES RECENT UNSEEN)\r\n"},
            {1, "A005 STORE 1:3 +FLAGS (\\Seen)\r\n"},
            {3, "C003 FETCH 1:* (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)\r\n"},
            {2, "B004 SEARCH FROM \"important\" SINCE 1-Jan-2023\r\n"},
            {1, "A006 LOGOUT\r\n"},
            {2, "B005 APPEND INBOX {310}\r\n"},
            {2, "From: sender@example.com\r\nTo: recipient@example.com\r\nSubject: Test Message\r\nDate: Mon, 7 Feb 2023 21:52:25 -0800\r\nMessage-Id: <B27397-0100000@example.com>\r\nMIME-Version: 1.0\r\nContent-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n\r\nThis is a test message.\r\n.\r\n"},
            {3, "C004 LOGOUT\r\n"},
            {2, "B006 LOGOUT\r\n"}
        };
        
        // 3. 处理所有命令
        std::cout << "\n===== 处理IMAP命令 =====" << std::endl;
        
        for (const auto& cmd : commands) {
            int flowId = cmd.first;
            const std::string& command = cmd.second;
            
            std::string clientIP, serverIP;
            int clientPort, serverPort;
            
            // 根据流ID选择对应的IP和端口
            switch (flowId) {
                case 1:
                    clientIP = clientIP1;
                    serverIP = serverIP1;
                    clientPort = clientPort1;
                    serverPort = serverPort1;
                    break;
                case 2:
                    clientIP = clientIP2;
                    serverIP = serverIP2;
                    clientPort = clientPort2;
                    serverPort = serverPort2;
                    break;
                case 3:
                    clientIP = clientIP3;
                    serverIP = serverIP3;
                    clientPort = clientPort3;
                    serverPort = serverPort3;
                    break;
            }
            
            std::cout << "\n处理流 #" << flowId << " 的IMAP命令: ";
            // 如果命令太长，只显示前50个字符
            if (command.length() > 50) {
                std::cout << command.substr(0, 47) << "...";
            } else {
                std::cout << command;
            }
            
            // 创建InputPacket并处理
            flow_table::InputPacket packet = createImapPacket(
                command, "C2S", clientIP, clientPort, serverIP, serverPort
            );
            
            // 处理数据包
            bool result = flowTable.processPacket(packet);
            
            // 输出处理结果
            std::cout << "处理结果: " << (result ? "成功" : "失败") << std::endl;
            
            // 模拟网络延迟
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // 4. 获取所有流并输出消息
        std::cout << "\n===== 输出所有流的消息 =====" << std::endl;
        std::vector<flow_table::Flow*> allFlows = flowTable.getAllFlows();
        std::cout << "总共有 " << allFlows.size() << " 个活跃流" << std::endl;
        
        for (size_t i = 0; i < allFlows.size(); ++i) {
            std::cout << "\n----- 流 #" << (i + 1) << " -----" << std::endl;
            printFourTuple(allFlows[i]->getC2STuple());
            allFlows[i]->outputMessages();
        }
        
        // 5. 清理资源
        std::cout << "\n===== 清理资源 =====" << std::endl;
        
        // 显示当前流表中的流数量
        std::cout << "流表中的流数量: " << flowTable.getTotalFlows() << std::endl;
        
        // 尝试使用超时检查清理流
        std::cout << "执行超时检查..." << std::endl;
        flowTable.checkAndCleanupTimeoutFlows();
        std::cout << "超时检查后流表中的流数量: " << flowTable.getTotalFlows() << std::endl;
        
        // 这里不需要手动清理，因为作用域结束时flowTable的析构函数会被调用
        std::cout << "准备通过析构函数清理所有流..." << std::endl;
    } // flowTable的析构函数在这里被调用，清理所有流
    
    std::cout << "析构函数已执行，所有流已被清理" << std::endl;
    std::cout << "测试完成" << std::endl;
    
    return 0;
}
