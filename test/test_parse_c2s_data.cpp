#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"

using namespace flow_table;

int main() {
    std::cout << "===== parseC2SData函数测试 =====" << std::endl;
    
    // 创建一个Flow对象用于测试
    FourTuple c2sTuple;
    c2sTuple.srcIPvN = 4;
    c2sTuple.srcIPv4 = inet_addr("192.168.1.100");
    c2sTuple.sourcePort = 12345;
    c2sTuple.dstIPvN = 4;
    c2sTuple.dstIPv4 = inet_addr("10.0.0.1");
    c2sTuple.destPort = 143;
    
    Flow flow(c2sTuple);
    
    // 测试数据 - 与test_imap_parsing.cpp中的相同，但APPEND命令按照正确的IMAP协议流程
    std::vector<std::string> commands = {
        "A001 LOGIN user password\r\n",
        "A002 SELECT INBOX\r\n",
        "B001 LOGIN admin secure_password\r\n",
        "A003 FETCH 1:5 (FLAGS BODY[HEADER])\r\n",
        "C001 LOGIN test test123\r\n",
        "B002 LIST \"\" *\r\n",
        "A004 FETCH 1 (BODY[TEXT])\r\n",
        "C002 SELECT \"Sent Items\"\r\n",
        "B003 STATUS INBOX (MESSAGES RECENT UNSEEN)\r\n",
        "A005 STORE 1:3 +FLAGS (\\Seen)\r\n",
        "C003 FETCH 1:* (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)\r\n",
        "B004 SEARCH FROM \"important\" SINCE 1-Jan-2023\r\n",
        "A006 LOGOUT\r\n",
        // APPEND命令分两步：先发送命令和字面量大小，然后发送邮件内容
        "B005 APPEND INBOX {310}\r\n",
        // 这里应该有服务器响应 "+ Ready for literal data\r\n"，但客户端不会看到这个响应
        "From: sender@example.com\r\nTo: recipient@example.com\r\nSubject: Test Message\r\nDate: Mon, 7 Feb 2023 21:52:25 -0800\r\nMessage-Id: <B27397-0100000@example.com>\r\nMIME-Version: 1.0\r\nContent-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n\r\nThis is a test message.\r\n.\r\n",
        "C004 LOGOUT\r\n",
        "B006 LOGOUT\r\n"
    };
    
    // 测试每个命令
    for (const auto& command : commands) {
        std::cout << "\n===== 测试命令 =====" << std::endl;
        std::cout << "命令: ";
        // 如果命令太长，只显示前50个字符
        if (command.length() > 50) {
            std::cout << command.substr(0, 47) << "...";
        } else {
            std::cout << command;
        }
        std::cout << std::endl;
        
        // 添加命令到Flow的C2S缓冲区
        flow.addC2SData(command);
        
        // 调用parseC2SData函数进行解析
        bool result = flow.parseC2SData();
        
        // 输出解析结果
        std::cout << "解析结果: " << (result ? "检测到LOGOUT命令" : "正常解析") << std::endl;
        
        // 输出当前所有消息
        std::cout << "当前消息输出:" << std::endl;
        flow.outputMessages();
    }
    
    std::cout << "\n===== 测试完成 =====" << std::endl;
    return 0;
}
