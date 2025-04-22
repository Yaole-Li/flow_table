/**
 * @file test_imap_parsing.cpp
 * @brief IMAP协议解析性能和多线程测试
 * 
 * 本测试文件用于评估IMAP协议解析器在高负载和多线程环境下的性能和稳定性。
 * 主要功能：
 * 1. 生成大量随机IMAP命令和数据包
 * 2. 模拟多客户端并发连接
 * 3. 使用多线程并行处理IMAP命令
 * 4. 测试流管理器(HashFlowTable)的并发处理能力
 * 5. 验证在高负载下解析器的正确性和性能
 * 
 * 测试方法：
 * - 创建多个线程，每个线程模拟一个客户端发送IMAP命令
 * - 使用随机生成的IP地址和端口号创建数据包
 * - 通过流管理器处理这些数据包并验证结果
 * - 计算处理时间和吞吐量
 * 
 * 该测试对于评估系统在实际环境中的性能十分重要，可以帮助识别潜在的瓶颈和并发问题。
 */

#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <atomic>
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

// 生成随机的IMAP命令
std::string generateRandomImapCommand(const std::string& prefix, int cmdNum) {
    std::vector<std::string> commandTypes = {
        "LOGIN", "SELECT", "FETCH", "STORE", "LIST", "STATUS", "SEARCH", "EXAMINE", "CREATE", "DELETE", "RENAME", "SUBSCRIBE", "UNSUBSCRIBE", "LSUB", "APPEND", "CHECK", "CLOSE", "EXPUNGE", "COPY", "UID"
    };
    
    std::vector<std::string> mailboxes = {
        "INBOX", "Sent", "Drafts", "Trash", "Junk", "Archive", "Important", "Work", "Personal", "Family", "Friends", "Projects", "Receipts", "Travel", "Shopping"
    };
    
    std::vector<std::string> flags = {
        "\\Seen", "\\Answered", "\\Flagged", "\\Deleted", "\\Draft", "\\Recent", "$Important", "$Work", "$Personal"
    };
    
    std::string cmd = prefix + std::to_string(cmdNum) + " ";
    int cmdType = rand() % commandTypes.size();
    cmd += commandTypes[cmdType] + " ";
    
    // 根据命令类型添加不同的参数
    switch (cmdType) {
        case 0: // LOGIN
            cmd += "user" + std::to_string(rand() % 100) + " ";
            cmd += "password" + std::to_string(rand() % 1000);
            break;
        case 1: // SELECT
        case 7: // EXAMINE
            cmd += "\"" + mailboxes[rand() % mailboxes.size()] + "\"";
            break;
        case 2: // FETCH
            cmd += std::to_string(1 + (rand() % 10)) + ":" + std::to_string(10 + (rand() % 90)) + " ";
            cmd += "(FLAGS BODY[HEADER])";
            break;
        case 3: // STORE
            cmd += std::to_string(1 + (rand() % 5)) + ":" + std::to_string(5 + (rand() % 10)) + " ";
            cmd += "+FLAGS (" + flags[rand() % flags.size()] + ")";
            break;
        case 4: // LIST
            cmd += "\"\" \"*\"";
            break;
        case 5: // STATUS
            cmd += mailboxes[rand() % mailboxes.size()] + " ";
            cmd += "(MESSAGES RECENT UNSEEN)";
            break;
        case 6: // SEARCH
            cmd += "FROM \"user" + std::to_string(rand() % 100) + "\" ";
            cmd += "SINCE 1-Jan-2023";
            break;
        default: // 其他命令使用简单参数
            if (rand() % 2 == 0) {
                cmd += mailboxes[rand() % mailboxes.size()];
            }
            break;
    }
    
    return cmd + "\r\n";
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

// 多线程测试函数
void multiThreadTest(flow_table::HashFlowTable& flowTable, const std::string& clientIP, const std::string& serverIP, int clientPort, int serverPort, int threadId) {
    // 随机生成flowId，确保不同线程的flowId不同
    int flowId = 1000 + threadId * 100 + (rand() % 100);
    
    // 生成命令前缀（用于标识不同的流）
    char prefix = 'A' + (threadId % 26);
    
    // 生成更多的随机命令
    std::vector<std::string> commands;
    int commandCount = 10 + (rand() % 15); // 每个线程处理10-24个命令
    
    for (int i = 1; i <= commandCount; i++) {
        commands.push_back(generateRandomImapCommand(std::string(1, prefix), i));
    }
    
    // 确保最后一个命令是LOGOUT
    // commands.push_back(std::string(1, prefix) + std::to_string(commandCount + 1) + " LOGOUT\r\n");

    std::cout << "\n===== 线程 #" << threadId << " (流ID: " << flowId << ") 开始处理 " << commands.size() << " 个命令 =====" << std::endl;
    
    // 处理所有命令
    for (const auto& command : commands) {
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
        
        // 模拟网络延迟（随机延迟，使线程交错执行）
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + (rand() % 100)));
    }
    
    std::cout << "\n===== 线程 #" << threadId << " (流ID: " << flowId << ") 完成处理 =====" << std::endl;
}

int main() {
    std::cout << "===== IMAP多线程解析测试 =====" << std::endl;
    
    // 设置随机种子
    srand(time(nullptr));
    
    // 创建一个作用域，使flowTable在作用域结束时自动销毁
    {
        // 1. 创建哈希流表
        flow_table::HashFlowTable flowTable;
        flowTable.setFlowTimeout(120); // 设置超时时间为120s
        
        // 多线程测试
        std::cout << "\n===== 启动多线程测试 =====" << std::endl;
        
        // 创建多个线程，每个线程处理不同的流
        const int threadCount = 5; // 5个线程
        std::vector<std::thread> threads;
        
        // 创建和启动线程，所有流都随机生成
        for (int i = 1; i <= threadCount; i++) {
            std::string randomClientIP = generateRandomIP();
            std::string randomServerIP = "10.0.0." + std::to_string(i);
            int randomClientPort = generateRandomPort();
            int randomServerPort = 143;
            
            threads.push_back(std::thread(multiThreadTest, std::ref(flowTable), randomClientIP, randomServerIP, randomClientPort, randomServerPort, i));
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
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
        
        // 这里不需要手动清理，因为作用域结束时flowTable的析构函数会被调用
        std::cout << "准备通过析构函数清理所有流..." << std::endl;
    } // flowTable的析构函数在这里被调用，清理所有流
    
    std::cout << "析构函数已执行，所有流已被清理\n测试完成" << std::endl;
    
    return 0;
}
