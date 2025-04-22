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
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"
#include "../include/tools/s2ctools.h"
#include "../include/config/config_parser.h"  // 添加配置文件解析器支持

using namespace flow_table;

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
    std::cout << "\n====== 测试FETCH命令响应解析 ======" << std::endl;
    
    // 创建哈希流表
    HashFlowTable flowTable;
    
    // 创建客户端到服务器的数据包 - 先发送LOGIN命令
    InputPacket c2sPacket1;
    c2sPacket1.type = "C2S";
    c2sPacket1.payload = "A001 LOGIN user password\r\n";
    
    // 设置四元组
    c2sPacket1.fourTuple.srcIPvN = 4;
    c2sPacket1.fourTuple.srcIPv4 = inet_addr("192.168.1.100");
    c2sPacket1.fourTuple.sourcePort = 12345;
    c2sPacket1.fourTuple.dstIPvN = 4;
    c2sPacket1.fourTuple.dstIPv4 = inet_addr("192.168.1.200");
    c2sPacket1.fourTuple.destPort = 143;
    
    // 处理C2S数据包 - LOGIN命令
    flowTable.processPacket(c2sPacket1);
    
    // 创建服务器到客户端的响应数据包 - LOGIN响应
    InputPacket s2cPacket1;
    s2cPacket1.type = "S2C";
    s2cPacket1.payload = "* OK IMAP server ready\r\nA001 OK LOGIN completed\r\n";
    
    // 设置四元组（注意源和目标交换）
    s2cPacket1.fourTuple.srcIPvN = 4;
    s2cPacket1.fourTuple.srcIPv4 = inet_addr("192.168.1.200");
    s2cPacket1.fourTuple.sourcePort = 143;
    s2cPacket1.fourTuple.dstIPvN = 4;
    s2cPacket1.fourTuple.dstIPv4 = inet_addr("192.168.1.100");
    s2cPacket1.fourTuple.destPort = 12345;
    
    // 处理S2C数据包 - LOGIN响应
    flowTable.processPacket(s2cPacket1);
    
    // 创建客户端到服务器的数据包 - 发送SELECT命令
    InputPacket c2sPacket2;
    c2sPacket2.type = "C2S";
    c2sPacket2.payload = "A002 SELECT INBOX\r\n";
    c2sPacket2.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - SELECT命令
    flowTable.processPacket(c2sPacket2);
    
    // 创建服务器到客户端的响应数据包 - SELECT响应
    InputPacket s2cPacket2;
    s2cPacket2.type = "S2C";
    s2cPacket2.payload = "* 1 EXISTS\r\n* 1 RECENT\r\n* OK [UNSEEN 1] First unseen.\r\n* OK [UIDVALIDITY 1461167108] UIDs valid\r\nA002 OK [READ-WRITE] SELECT completed\r\n";
    s2cPacket2.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - SELECT响应
    flowTable.processPacket(s2cPacket2);
    
    // 创建客户端到服务器的数据包 - 发送FETCH命令
    InputPacket c2sPacket3;
    c2sPacket3.type = "C2S";
    c2sPacket3.payload = "A003 FETCH 1 (BODY[HEADER] BODY[TEXT])\r\n";
    c2sPacket3.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - FETCH命令
    flowTable.processPacket(c2sPacket3);
    
    // 创建服务器到客户端的响应数据包 - FETCH响应（前两封邮件）
    InputPacket s2cPacket3;
    s2cPacket3.type = "S2C";
    s2cPacket3.payload = "* 1 FETCH (FLAGS (\\Seen) INTERNALDATE \"11-Nov-1999 08:36:54 -0600\" RFC822.SIZE 2059 BODY[HEADER.FIELDS (DATE FROM SUBJECT TO CC MESSAGE-ID REFERENCES CONTENT-TYPE IN-REPLY-TO REPLY-TO)] {303}\r\n"
                         "Date: Thu, 11 Nov 1999 08:36:54 -0600\r\n"
                         "From: \"Uetrecht, Daniel J.\" <uetrecht@umr.edu>\r\n"
                         "Subject: RE: pipes implementation of bkupexec agent\r\n"
                         "To: \"Neulinger, Nathan R.\" <nneul@umr.edu>\r\n"
                         "Message-ID: <9DA8D24B915BD1118911006094516EAF0261940C@umr-mail02>\r\n"
                         "Content-Type: text/plain;\r\n"
                         "\tcharset=\"iso-8859-1\"\r\n"
                         "\r\n"
                         ")\r\n"
                         "* 2 FETCH (FLAGS (\\Seen) INTERNALDATE \"11-Nov-1999 08:42:21 -0600\" RFC822.SIZE 3151 BODY[HEADER.FIELDS (DATE FROM SUBJECT TO CC MESSAGE-ID REFERENCES CONTENT-TYPE IN-REPLY-TO REPLY-TO)] {303}\r\n"
                         "Date: Thu, 11 Nov 1999 08:42:20 -0600\r\n"
                         "From: \"Uetrecht, Daniel J.\" <uetrecht@umr.edu>\r\n"
                         "Subject: RE: pipes implementation of bkupexec agent\r\n"
                         "To: \"Neulinger, Nathan R.\" <nneul@umr.edu>\r\n"
                         "Message-ID: <9DA8D24B915BD1118911006094516EAF0261940D@umr-mail02>\r\n"
                         "Content-Type: text/plain;\r\n"
                         "\tcharset=\"iso-8859-1\"\r\n"
                         "\r\n"
                         ")\r\n"
                         "* 3 FETCH (FLAGS (\\Seen))\r\n"
                         "A003 OK FETCH completed\r\n";
    s2cPacket3.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - FETCH响应（前两封邮件）
    flowTable.processPacket(s2cPacket3);
    
    // 创建新的服务器到客户端的响应数据包 - 中文邮件头部
    InputPacket s2cPacket4;
    s2cPacket4.type = "S2C";
    
    // 定义中文邮件头部内容
    std::string headerContent = 
        "Date: Mon, 21 Apr 2025 12:30:00 +0800\r\n"
        "From: \"小明\" <xiaoming@example.com>\r\n"
        "Subject: 测试邮件\r\n"
        "To: \"张三\" <zhangsan@example.com>\r\n"
        "Cc: \"王五\" <wangwu@example.com>\r\n"
        "Message-ID: <20250421123000.GA23456@example.com>\r\n"
        "Content-Type: text/plain;\r\n"
        "\tcharset=\"utf-8\"\r\n"
        "\r\n";
    
    // 计算头部内容的字节数
    size_t headerSize = calculateLiteralSize(headerContent);
    
    // 构建头部FETCH响应
    s2cPacket4.payload = "* 4 FETCH (FLAGS (\\Seen) INTERNALDATE \"21-Apr-2025 12:30:00 +0800\" "
                         "RFC822.SIZE 2288 "
                         "BODY[HEADER] {" + std::to_string(headerSize) + "}\r\n" +
                         headerContent +
                         ")\r\n";
    s2cPacket4.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 中文邮件头部
    flowTable.processPacket(s2cPacket4);
    
    // 创建新的服务器到客户端的响应数据包 - 中文邮件正文
    InputPacket s2cPacket5;
    s2cPacket5.type = "S2C";
    
    // 定义中文邮件正文内容
    std::string bodyContent = 
        "尊敬的张三先生：\r\n"
        "\r\n"
        "您好！这是一封测试邮件，用于测试IMAP协议的中文邮件解析功能。\r\n"
        "\r\n"
        "祝好！\r\n"
        "李小明\r\n"
        "邮件技术部\r\n"
        "2025年4月21日\r\n";
    
    // 计算正文内容的字节数
    size_t bodySize = calculateLiteralSize(bodyContent);
    
    // 构建正文FETCH响应
    s2cPacket5.payload = "* 4 FETCH (BODY[TEXT] {" + std::to_string(bodySize) + "}\r\n" +
                         bodyContent +
                         ")\r\n" +
                         "A004 OK FETCH completed\r\n";
    s2cPacket5.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 中文邮件正文
    flowTable.processPacket(s2cPacket5);
    
    // 测试长邮件解析 - 从input.txt文件读取内容
    std::cout << "\n====== 测试长邮件解析 ======" << std::endl;
    
    // 创建新的客户端到服务器的请求数据包 - FETCH命令
    InputPacket c2sPacket4;
    c2sPacket4.type = "C2S";
    c2sPacket4.payload = "A005 FETCH 5 (BODY[HEADER] BODY[TEXT])\r\n";
    c2sPacket4.fourTuple = c2sPacket1.fourTuple; // 使用相同的四元组
    
    // 处理C2S数据包 - FETCH命令
    flowTable.processPacket(c2sPacket4);
    
    // 定义长邮件头部内容
    std::string longMailHeaderContent = 
        "Date: Mon, 22 Apr 2025 14:00:00 +0800\r\n"
        "From: \"研究团队\" <research@example.com>\r\n"
        "Subject: 关于社区搜索算法的研究报告\r\n"
        "To: \"技术部门\" <tech@example.com>\r\n"
        "Cc: \"管理层\" <management@example.com>\r\n"
        "Message-ID: <20250422140000.GA12345@example.com>\r\n"
        "Content-Type: text/plain;\r\n"
        "\tcharset=\"utf-8\"\r\n"
        "\r\n";
    
    // 读取input.txt文件的内容作为长邮件正文
    std::ifstream inputFile("/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/auto_AC/file/input.txt");
    std::string longMailBodyContent;
    if (inputFile.is_open()) {
        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        longMailBodyContent = buffer.str();
        inputFile.close();
    } else {
        std::cerr << "无法打开input.txt文件" << std::endl;
        longMailBodyContent = "无法读取文件内容。这是一个替代的长邮件正文，用于测试解析功能。\r\n";
    }
    
    // 创建长邮件的头部响应数据包
    InputPacket s2cPacket6;
    s2cPacket6.type = "S2C";
    
    // 计算头部内容的字节数
    size_t longHeaderSize = calculateLiteralSize(longMailHeaderContent);
    
    // 构建头部FETCH响应
    s2cPacket6.payload = "* 5 FETCH (FLAGS (\\Seen) INTERNALDATE \"22-Apr-2025 14:00:00 +0800\" "
                         "RFC822.SIZE 15000 "
                         "BODY[HEADER] {" + std::to_string(longHeaderSize) + "}\r\n" +
                         longMailHeaderContent +
                         ")\r\n";
    s2cPacket6.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 长邮件头部
    flowTable.processPacket(s2cPacket6);
    
    // 创建长邮件的正文响应数据包
    InputPacket s2cPacket7;
    s2cPacket7.type = "S2C";
    
    // 计算正文内容的字节数
    size_t longBodySize = calculateLiteralSize(longMailBodyContent);
    
    // 构建正文FETCH响应
    s2cPacket7.payload = "* 5 FETCH (BODY[TEXT] {" + std::to_string(longBodySize) + "}\r\n" +
                         longMailBodyContent +
                         ")\r\n" +
                         "A005 OK FETCH completed\r\n";
    s2cPacket7.fourTuple = s2cPacket1.fourTuple; // 使用相同的四元组
    
    // 处理S2C数据包 - 长邮件正文
    flowTable.processPacket(s2cPacket7);
    
    // 输出所有流的处理结果
    std::cout << "\n===== 解析结果输出 =====" << std::endl;
    flowTable.outputResults();
    
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
    if (!config.loadFromFile("config.ini")) {
        std::cerr << "警告: 无法加载配置文件 config.ini，将使用默认值" << std::endl;
    }
    
    // 获取邮件内容保存路径
    std::string emailContentPathRelative = config.getString("Paths.test_email_content", "test/parsed_email_content.txt");
    std::string parsedFileName = "/Users/liyaole/Documents/works/c_work/imap_works/flow_table/" + emailContentPathRelative;
    
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
