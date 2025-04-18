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

using namespace flow_table;

// 添加全局互斥锁，用于保护对共享资源的访问
std::mutex flowTableMutex;

// 创建一个InputPacket结构体，模拟IMAP命令数据包
InputPacket createImapPacket(const std::string& payload, const std::string& type, 
                             const std::string& srcIP, int srcPort, 
                             const std::string& dstIP, int dstPort) {
    InputPacket packet;
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
    int a = 10 + (rand() % 245);
    int b = 1 + (rand() % 254);
    int c = 1 + (rand() % 254);
    int d = 2 + (rand() % 253);
    
    return std::to_string(a) + "." + std::to_string(b) + "." + 
           std::to_string(c) + "." + std::to_string(d);
}

// 生成一个随机端口号
int generateRandomPort() {
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

// 多线程测试函数
void multiThreadTest(HashFlowTable& flowTable, const std::string& clientIP, const std::string& serverIP, int clientPort, int serverPort, int threadId) {
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
    
    // 处理所有命令
    for (const auto& command : commands) {
        // 创建InputPacket并处理
        InputPacket packet = createImapPacket(
            command, "C2S", clientIP, clientPort, serverIP, serverPort
        );
        
        // 使用互斥锁保护对flowTable的访问
        {
            std::lock_guard<std::mutex> lock(flowTableMutex);
            flowTable.processPacket(packet);
        }
        
        // 模拟网络延迟（随机延迟，使线程交错执行）
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + (rand() % 20)));
    }
}

int main() {
    // 记录开始时间
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 设置随机种子
    srand(time(nullptr));
    
    // 创建一个作用域，使flowTable在作用域结束时自动销毁
    {
        // 创建哈希流表
        HashFlowTable flowTable;
        flowTable.setFlowTimeout(120); // 超时时间 120ms
        
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
        
    } // flowTable的析构函数在这里被调用，清理所有流
    
    // 记录结束时间
    auto endTime = std::chrono::high_resolution_clock::now();
    
    // 计算运行时间（毫秒）
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 输出运行时间
    std::cout << "总运行时间: " << duration.count() << " 毫秒" << std::endl;
    
    return 0;
}
