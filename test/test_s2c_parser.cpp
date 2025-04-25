/**
 * @file test_s2c_parser.cpp
 * @brief IMAP协议服务器到客户端(S2C)解析器测试
 * 
 * 本测试文件用于验证IMAP协议服务器响应的解析功能。
 * 主要功能：
 * 1. 测试Base64解码和编码转换功能
 * 2. 测试IMAP协议正文解析能力
 * 3. 测试S2C解析器对不同类型IMAP响应的处理
 * 4. 模拟完整IMAP会话，包括：
 *    - 普通英文邮件解析
 *    - 中文邮件解析（支持UTF-8编码）
 *    - 长邮件解析
 *    - 提取和保存解析后的邮件内容
 * 
 * 使用配置文件(config.ini)获取以下参数：
 * - 解析后的邮件内容保存路径
 * 
 * 本测试是IMAP邮件关键词检测系统的基础部分，负责从原始协议数据中提取有意义的邮件内容。
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>  // 用于文件操作
#include <sstream>  // 用于字符串流
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include <limits.h>
#include <unistd.h>
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"
#include "../include/tools/s2ctools.h"
#include "../include/config/config_parser.h"  // 添加配置文件解析器支持

using namespace flow_table;

// 获取项目根目录的工具函数
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

// 用于计算IMAP响应中文字量(literal)的准确字节数
size_t calculateLiteralSize(const std::string& content) {
    return content.length();
}

// 保存邮件内容到文件
bool saveEmailContent(const std::string& content, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    outFile << content;
    outFile.close();
    std::cout << "已将邮件内容保存到文件: " << filename << std::endl;
    return true;
}

// 创建一个Flow对象用于测试
Flow* createTestFlow() {
    // 创建一个四元组，模拟客户端到服务器的连接
    FourTuple c2sTuple;
    c2sTuple.srcIPvN = 4;  // IPv4
    c2sTuple.srcIPv4 = inet_addr("192.168.1.100");
    c2sTuple.sourcePort = 12345;
    c2sTuple.dstIPvN = 4;  // IPv4
    c2sTuple.dstIPv4 = inet_addr("192.168.1.200");
    c2sTuple.destPort = 143;  // IMAP端口
    
    // 创建Flow对象
    return new Flow(c2sTuple);
}

// 打印Email对象的内容
void printEmail(const Email& email) {
    std::cout << "========== Email信息 ==========" << std::endl;
    std::cout << "序列号: " << email.sequence_number << std::endl;
    std::cout << "UID: " << email.uid << std::endl;
    std::cout << "RFC822大小: " << email.rfc822_size << std::endl;
    
    if (!email.internaldate.empty()) {
        std::cout << "内部日期: " << email.internaldate << std::endl;
    }
    
    if (!email.flags.empty()) {
        std::cout << "标志: " << email.flags << std::endl;
    }
    
    if (!email.envelope.empty()) {
        std::cout << "信封: " << email.envelope << std::endl;
    }
    
    if (!email.bodystructure.empty()) {
        std::cout << "正文结构: " << email.bodystructure << std::endl;
    }
    
    if (!email.body.text.empty()) {
        std::cout << "正文内容: " << email.body.text << std::endl;
    }
    
    // 打印头部信息
    if (!email.body.header.from.empty()) {
        std::cout << "发件人: " << email.body.header.from << std::endl;
    }
    
    if (!email.body.header.to.empty()) {
        std::cout << "收件人: ";
        for (const auto& to : email.body.header.to) {
            std::cout << to << " ";
        }
        std::cout << std::endl;
    }
    
    if (!email.body.header.subject.empty()) {
        std::cout << "主题: ";
        for (const auto& subj : email.body.header.subject) {
            std::cout << subj << " ";
        }
        std::cout << std::endl;
    }
    
    if (!email.body.header.date.empty()) {
        std::cout << "日期: " << email.body.header.date << std::endl;
    }
    
    std::cout << "===============================" << std::endl;
}

// 打印Message对象的内容
void printMessage(const Message& message) {
    std::cout << "========== 消息信息 ==========" << std::endl;
    std::cout << "标签: " << message.tag << std::endl;
    std::cout << "命令: " << message.command << std::endl;
    
    std::cout << "参数: ";
    for (const auto& arg : message.args) {
        std::cout << arg << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Fetch响应数量: " << message.fetch.size() << std::endl;
    for (size_t i = 0; i < message.fetch.size(); ++i) {
        std::cout << "Fetch响应 #" << (i+1) << ":" << std::endl;
        printEmail(message.fetch[i]);
    }
    
    std::cout << "===============================" << std::endl;
}

// 测试Base64解码和编码转换功能
void testBase64AndEncoding() {
    std::cout << "\n====== 测试Base64解码和编码转换 ======" << std::endl;
    
    // 测试Base64解码
    std::string encoded_string = "DQrmiJHnu4Plip/lj5Hoh6rnnJ/lv4M="; // Base64编码的中文文本
    std::string decoded_string = Base64_decode(encoded_string);
    
    std::cout << "编码字符串: " << encoded_string << std::endl;
    std::cout << "解码字符串: " << decoded_string << std::endl;
    
    // 测试GBK和UTF8转换
    std::string utf8_string = Gbk_to_utf8(decoded_string);
    std::cout << "UTF-8转换后: " << utf8_string << std::endl;
    
    std::string gbk_string = Utf8_to_gbk(utf8_string);
    std::cout << "GBK转换后 (十六进制): ";
    for (unsigned char c : gbk_string) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << std::endl;
}

// 测试IMAP正文解析功能
void testImapBodyParsing() {
    std::cout << "\n====== 测试IMAP正文解析 ======" << std::endl;
    
    // 创建一个简单的IMAP邮件正文
    std::string emailBody = 
        "From: sender@example.com\r\n"
        "To: recipient@example.com\r\n"
        "Subject: Test Email\r\n"
        "Date: Fri, 18 Apr 2025 10:00:00 +0800\r\n"
        "Content-Type: text/plain; charset=\"utf-8\"\r\n"
        "\r\n"
        "这是一封测试邮件的正文内容。\r\n"
        "Hello, World!\r\n";
    
    // 创建Email对象
    Email testEmail;
    
    // 解析邮件正文
    int result = Resolve_imap_body(emailBody, testEmail, true, true);
    
    std::cout << "解析结果: " << (result == 0 ? "成功" : "失败") << std::endl;
    printEmail(testEmail);
}

// 测试S2C解析器
void testS2CParser() {
    std::cout << "\n====== 测试S2C解析器 ======" << std::endl;
    
    // 创建Flow对象
    Flow* testFlow = createTestFlow();
    
    // 添加一个简单的IMAP FETCH响应数据
    std::string fetchResponse = 
        "* 1 FETCH (UID 100 FLAGS (\\Seen) INTERNALDATE \"18-Apr-2025 10:00:00 +0800\" "
        "RFC822.SIZE 1024 BODY[HEADER] {158}\r\n"
        "From: sender@example.com\r\n"
        "To: recipient@example.com\r\n"
        "Subject: Test Email\r\n"
        "Date: Fri, 18 Apr 2025 10:00:00 +0800\r\n"
        "Content-Type: text/plain; charset=\"utf-8\"\r\n"
        "\r\n"
        ")\r\n";
    
    // 添加到S2C缓冲区
    testFlow->addS2CData(fetchResponse);
    
    // 解析S2C数据
    bool parseResult = testFlow->parseS2CData();
    
    std::cout << "S2C解析结果: " << (parseResult ? "成功" : "失败") << std::endl;
    
    // 输出解析后的消息
    testFlow->outputMessages();
    
    // 清理资源
    delete testFlow;
}

// 测试完整的IMAP会话
void testCompleteImapSession() {
    std::cout << "\n====== 测试完整IMAP会话 ======" << std::endl;
    
    // 创建哈希流表
    HashFlowTable flowTable;
    
    // 创建客户端到服务器的数据包 - 先发送LOGIN命令
    InputPacket c2sPacket1;
    c2sPacket1.type = "C2S";
    c2sPacket1.payload = "a1 login 1094825151@qq.com mlqecbulvgjjxhdf\r\n";
    
    // 设置四元组
    c2sPacket1.fourTuple.srcIPvN = 4;
    c2sPacket1.fourTuple.srcIPv4 = inet_addr("192.168.1.100");
    c2sPacket1.fourTuple.sourcePort = 12345;
    c2sPacket1.fourTuple.dstIPvN = 4;
    c2sPacket1.fourTuple.dstIPv4 = inet_addr("10.0.0.1");
    c2sPacket1.fourTuple.destPort = 143;
    
    // 处理C2S数据包 - LOGIN命令
    flowTable.processPacket(c2sPacket1);
    
    // 创建服务器到客户端的响应数据包 - LOGIN响应
    InputPacket s2cPacket1;
    s2cPacket1.type = "S2C";
    s2cPacket1.payload = "* OK [CAPABILITY IMAP4 IMAP4rev1 ID AUTH=PLAIN AUTH=LOGIN AUTH=XOAUTH2 NAMESPACE] QQMail XMIMAP4Server ready\r\n"
                         "a1 OK Success login ok\r\n";
    
    // 设置四元组（注意源和目标交换）
    s2cPacket1.fourTuple.srcIPvN = 4;
    s2cPacket1.fourTuple.srcIPv4 = inet_addr("10.0.0.1");
    s2cPacket1.fourTuple.sourcePort = 143;
    s2cPacket1.fourTuple.dstIPvN = 4;
    s2cPacket1.fourTuple.dstIPv4 = inet_addr("192.168.1.100");
    s2cPacket1.fourTuple.destPort = 12345;
    
    // 处理S2C数据包 - LOGIN响应
    flowTable.processPacket(s2cPacket1);
    
    // 创建客户端到服务器的数据包 - 发送SELECT命令
    InputPacket c2sPacket2;
    c2sPacket2.type = "C2S";
    c2sPacket2.payload = "a2 SELECT \"INBOX\"\r\n";
    c2sPacket2.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - SELECT命令
    flowTable.processPacket(c2sPacket2);
    
    // 创建服务器到客户端的响应数据包 - SELECT响应
    InputPacket s2cPacket2;
    s2cPacket2.type = "S2C";
    s2cPacket2.payload = "* 7 EXISTS\r\n"
                         "* 0 RECENT\r\n"
                         "* OK [UNSEEN 7]\r\n"
                         "* OK [UIDVALIDITY 1743484216] UID validity status\r\n"
                         "* OK [UIDNEXT 34] Predicted next UID\r\n"
                         "* FLAGS (\\Answered \\Flagged \\Deleted \\Draft \\Seen)\r\n"
                         "* OK [PERMANENTFLAGS (\\* \\Answered \\Flagged \\Deleted \\Draft \\Seen)] Permanent flags\r\n"
                         "a2 OK [READ-WRITE] SELECT complete\r\n";
    s2cPacket2.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - SELECT响应
    flowTable.processPacket(s2cPacket2);
    
    // 创建客户端到服务器的数据包 - 发送FETCH命令(邮件头部)
    InputPacket c2sPacket3;
    c2sPacket3.type = "C2S";
    c2sPacket3.payload = "a3 FETCH 1:10 (UID FLAGS BODY.PEEK[HEADER.FIELDS (FROM SUBJECT DATE)])\r\n";
    c2sPacket3.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - FETCH命令
    flowTable.processPacket(c2sPacket3);
    
    // 创建服务器到客户端的响应数据包 - FETCH响应（所有邮件的头部）
    InputPacket s2cPacket3;
    s2cPacket3.type = "S2C";
    s2cPacket3.payload = 
        "* 1 FETCH (UID 26 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {114}\r\n"
        "From: =?utf-8?B?5aeTIOWQjQ==?= <z1459384884@outlook.com>\r\n"
        "Subject: 11111\r\n"
        "Date: Tue, 8 Apr 2025 12:53:48 +0000\r\n"
        "\r\n"
        ")\r\n"
        "* 2 FETCH (UID 27 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {114}\r\n"
        "From: \"z1459384884@outlook.com\" <z1459384884@outlook.com>\r\n"
        "Subject: 2222\r\n"
        "Date: Tue, 8 Apr 2025 13:00:40 +0000\r\n"
        "\r\n"
        ")\r\n"
        "* 3 FETCH (UID 28 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {211}\r\n"
        "From: \"=?gb18030?B?hE0=?=\" <1094825151@qq.com>\r\n"
        "Subject: =?gb18030?B?16q3oqO6uaSzzMLXwO2089f30rUg1dTM7MP6LVMz?=\r\n"
        " =?gb18030?B?MjQwNjcwOTgttPPK/b7dyrG0+rXEyv2+3cDE08M=?=\r\n"
        "Date: Sat, 12 Apr 2025 16:52:10 +0800\r\n"
        "\r\n"
        ")\r\n"
        "* 4 FETCH (UID 29 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {132}\r\n"
        "From: \"=?gb18030?B?zOyyxQ==?=\" <1459384884@qq.com>\r\n"
        "Subject:  Vergil,give me the Yamato. \r\n"
        "Date: Thu, 24 Apr 2025 15:08:59 +0800\r\n"
        "\r\n"
        ")\r\n"
        "* 5 FETCH (UID 30 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {146}\r\n"
        "From: \"=?gb18030?B?zOyyxQ==?=\" <1459384884@qq.com>\r\n"
        "Subject: =?gb18030?B?zqy8qrb7o6yw0dHWxKe1trj4ztI=?=\r\n"
        "Date: Thu, 24 Apr 2025 15:15:48 +0800\r\n"
        "\r\n"
        ")\r\n"
        "* 6 FETCH (UID 31 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {134}\r\n"
        "From: \"=?gb18030?B?zOyyxQ==?=\" <1459384884@qq.com>\r\n"
        "Subject: =?gb18030?B?zfvCrsm9xtmyvA==?=\r\n"
        "Date: Fri, 25 Apr 2025 13:13:48 +0800\r\n"
        "\r\n"
        ")\r\n"
        "* 7 FETCH (UID 33 FLAGS () BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {108}\r\n"
        "From: \"=?gb18030?B?zOyyxQ==?=\" <1459384884@qq.com>\r\n"
        "Subject: joke\r\n"
        "Date: Fri, 25 Apr 2025 13:24:19 +0800\r\n"
        "\r\n"
        ")\r\n"
        "a3 OK FETCH Completed\r\n";
    s2cPacket3.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - FETCH响应（邮件头部）
    flowTable.processPacket(s2cPacket3);
    
    // 创建客户端到服务器的数据包 - 请求中文邮件头部
    InputPacket c2sPacket4;
    c2sPacket4.type = "C2S";
    c2sPacket4.payload = "a4 fetch 6 body.peek[header]\r\n";
    c2sPacket4.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - 请求中文邮件头部
    flowTable.processPacket(c2sPacket4);
    
    // 我们替换原始邮件内容为"望庐山瀑布"的内容
    std::string chineseEmailHeader = 
        "X-QQ-FEAT: zaIfg0hwV2qyNXeKamWrfK4JWb7Mc2tY\r\n"
        "X-QQ-SSF: 0001000000000010000000000000\r\n"
        "X-QQ-XMRINFO: M8wFrcb6n6Ii4I6kYxweyY8=\r\n"
        "X-HAS-ATTACH: no\r\n"
        "X-QQ-BUSINESS-ORIGIN: 2\r\n"
        "X-Originating-IP: 222.171.77.236\r\n"
        "X-QQ-STYLE: \r\n"
        "From: \"李白\" <libai@poetry.com>\r\n"
        "To: \"杜甫\" <dufu@poetry.com>\r\n"
        "Subject: 望庐山瀑布\r\n"
        "Mime-Version: 1.0\r\n"
        "Content-Type: text/plain; charset=\"utf-8\"\r\n"
        "Content-Transfer-Encoding: 8bit\r\n"
        "Date: Fri, 25 Apr 2025 13:13:48 +0800\r\n"
        "X-Priority: 3\r\n"
        "Message-ID: <tencent_33A1BEE979FC8E51E19BF951B1B16DA33707@qq.com>\r\n"
        "X-Mailer: QQMail 2.x\r\n";
    
    std::string chineseEmailBody = "\r\n日照香炉生紫烟，遥看瀑布挂前川。飞流直下三千尺，疑是银河落九天。\r\n";
        
    // 计算邮件头部的长度
    size_t chineseHeaderSize = chineseEmailHeader.length();
    
    // 创建FETCH响应 - 邮件头部
    InputPacket s2cPacket4;
    s2cPacket4.type = "S2C";
    s2cPacket4.payload = "* 6 FETCH (BODY[HEADER] {" + std::to_string(chineseHeaderSize) + "}\r\n" +
                         chineseEmailHeader +
                         ")\r\n"
                         "a4 OK FETCH Completed\r\n";
    s2cPacket4.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 中文邮件头部
    flowTable.processPacket(s2cPacket4);
    
    // 创建客户端到服务器的数据包 - 请求中文邮件正文
    InputPacket c2sPacket4b;
    c2sPacket4b.type = "C2S";
    c2sPacket4b.payload = "a4b fetch 6 body.peek[text]\r\n";
    c2sPacket4b.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - 请求中文邮件正文
    flowTable.processPacket(c2sPacket4b);
    
    // 计算邮件正文长度
    size_t chineseBodySize = chineseEmailBody.length();
    
    // 创建服务器到客户端的响应数据包 - 中文邮件正文
    InputPacket s2cPacket4b;
    s2cPacket4b.type = "S2C";
    s2cPacket4b.payload = "* 6 FETCH (BODY[TEXT] {" + std::to_string(chineseBodySize) + "}\r\n" +
                         chineseEmailBody +
                         ")\r\n"
                         "a4b OK FETCH Completed\r\n";
    s2cPacket4b.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 中文邮件正文
    flowTable.processPacket(s2cPacket4b);
    
    // 创建客户端到服务器的数据包 - 请求英文邮件头部
    InputPacket c2sPacket5;
    c2sPacket5.type = "C2S";
    c2sPacket5.payload = "a5 fetch 7 body.peek[header]\r\n";
    c2sPacket5.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - 请求英文邮件头部
    flowTable.processPacket(c2sPacket5);
    
    // 我们替换原始邮件内容为"joke"的内容
    std::string englishEmailHeader = 
        "X-QQ-FEAT: zaIfg0hwV2qyNXeKamWrfK4JWb7Mc2tY\r\n"
        "X-QQ-SSF: 0001000000000010000000000000\r\n"
        "X-QQ-XMRINFO: M/715EihBoGSf6IYSX1iLFg=\r\n"
        "X-HAS-ATTACH: no\r\n"
        "X-QQ-BUSINESS-ORIGIN: 2\r\n"
        "X-Originating-IP: 222.171.77.236\r\n"
        "X-QQ-STYLE: \r\n"
        "From: \"John\" <john@example.com>\r\n"
        "To: \"Mary\" <mary@example.com>\r\n"
        "Subject: joke\r\n"
        "Mime-Version: 1.0\r\n"
        "Content-Type: text/plain; charset=\"utf-8\"\r\n"
        "Content-Transfer-Encoding: 8bit\r\n"
        "Date: Fri, 25 Apr 2025 13:24:19 +0800\r\n"
        "X-Priority: 3\r\n"
        "Message-ID: <tencent_DE625DDBD36FCD7D11F3AE1DDBC2B7F30B08@qq.com>\r\n"
        "X-Mailer: QQMail 2.x\r\n";
    
    std::string englishEmailBody = "\r\nWhy don't skeletons fight each other? They don't have the guts!\r\n";
    
    // 计算英文邮件头部的长度
    size_t englishHeaderSize = englishEmailHeader.length();
    
    // 创建FETCH响应 - 英文邮件头部
    InputPacket s2cPacket5;
    s2cPacket5.type = "S2C";
    s2cPacket5.payload = "* 7 FETCH (BODY[HEADER] {" + std::to_string(englishHeaderSize) + "}\r\n" +
                         englishEmailHeader +
                         ")\r\n"
                         "a5 OK FETCH Completed\r\n";
    s2cPacket5.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 英文邮件头部
    flowTable.processPacket(s2cPacket5);
    
    // 创建客户端到服务器的数据包 - 请求英文邮件正文
    InputPacket c2sPacket5b;
    c2sPacket5b.type = "C2S";
    c2sPacket5b.payload = "a5b fetch 7 body.peek[text]\r\n";
    c2sPacket5b.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - 请求英文邮件正文
    flowTable.processPacket(c2sPacket5b);
    
    // 计算英文邮件正文长度
    size_t englishBodySize = englishEmailBody.length();
    
    // 创建服务器到客户端的响应数据包 - 英文邮件正文
    InputPacket s2cPacket5b;
    s2cPacket5b.type = "S2C";
    s2cPacket5b.payload = "* 7 FETCH (BODY[TEXT] {" + std::to_string(englishBodySize) + "}\r\n" +
                         englishEmailBody +
                         ")\r\n"
                         "a5b OK FETCH Completed\r\n";
    s2cPacket5b.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 英文邮件正文
    flowTable.processPacket(s2cPacket5b);
    
    // 创建客户端到服务器的数据包 - 发送LOGOUT命令
    InputPacket c2sPacket6;
    c2sPacket6.type = "C2S";
    c2sPacket6.payload = "a6 logout\r\n";
    c2sPacket6.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - LOGOUT命令
    flowTable.processPacket(c2sPacket6);
    
    // 创建服务器到客户端的响应数据包 - LOGOUT响应
    InputPacket s2cPacket6;
    s2cPacket6.type = "S2C";
    s2cPacket6.payload = "* BYE LOGOUT received\r\n"
                         "a6 OK LOGOUT Completed\r\n";
    s2cPacket6.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - LOGOUT响应
    flowTable.processPacket(s2cPacket6);
    
    // 输出所有流的处理结果
    std::cout << "\n===== 解析结果输出 =====" << std::endl;
    flowTable.outputResults();
    
    // 获取项目根目录
    std::string projectRoot = getProjectRoot();
    
    // 捕获解析后的邮件内容
    std::ostringstream outputCapture;
    std::streambuf* originalCout = std::cout.rdbuf();
    std::cout.rdbuf(outputCapture.rdbuf());
    
    // 再次调用输出函数，但输出到我们的捕获流中
    flowTable.outputResults();
    
    // 恢复标准输出
    std::cout.rdbuf(originalCout);
    
    // 获取捕获的输出内容作为解析后的邮件内容
    std::string parsedContent = outputCapture.str();
    
    // 从配置文件读取保存路径
    ConfigParser config;
    if (!config.loadFromFile(getProjectRoot() + "config.ini")) {
        std::cerr << "警告: 无法加载配置文件 " << getProjectRoot() + "config.ini" << "，将使用默认值" << std::endl;
    }
    
    // 获取邮件内容保存路径
    std::string emailContentPathRelative = config.getString("Paths.test_email_content", "test/parsed_email_content.txt");
    std::string parsedFileName;
    
    // 检查是否已经是绝对路径
    if (emailContentPathRelative.empty()) {
        parsedFileName = projectRoot + "test/parsed_email_content.txt";
    } else if (emailContentPathRelative[0] == '/') {
        // 已经是绝对路径，直接使用
        parsedFileName = emailContentPathRelative;
    } else {
        // 相对路径，添加项目根目录
        parsedFileName = projectRoot + emailContentPathRelative;
    }
    
    // 保存解析后的邮件内容到文件
    saveEmailContent(parsedContent, parsedFileName);
    std::cout << "\n已将解析后的邮件内容保存到: " << parsedFileName << " 供关键词检测使用" << std::endl;
}

int main() {
    std::cout << "开始测试S2C解析器..." << std::endl;
    
    // 测试Base64解码和编码转换
    // testBase64AndEncoding();
    
    // 测试IMAP正文解析
    // testImapBodyParsing();
    
    // 测试S2C解析器
    // testS2CParser();
    
    // 测试完整的IMAP会话
    testCompleteImapSession();
    
    std::cout << "测试完成!" << std::endl;
    return 0;
}
