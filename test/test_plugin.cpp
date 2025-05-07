/**
 * @file test_plugin.cpp
 * @brief 用于测试IMAP流量分析和关键词检测插件的程序
 * 
 * 该程序加载imap_plugin共享库，并调用其提供的四个主要接口函数：
 * - GlobalInit
 * - ThreadInit
 * - Filter
 * - Remove
 * 
 * 程序将模拟C2S和S2C数据包，类似于main_of_0x12.cpp中的测试方式。
 */

#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <dlfcn.h>
#include "../include/plugin/plugin.h"

// 辅助函数：打印使用帮助
void printUsage(const char* programName) {
    std::cout << "使用方法: " << programName << " [配置文件路径]" << std::endl;
    std::cout << "  如果未指定配置文件路径，将使用默认搜索路径" << std::endl;
}

// 主函数
int main(int argc, char* argv[]) {
    std::cout << "===== IMAP流量分析和关键词检测插件测试程序 =====" << std::endl;
    
    // 处理命令行参数
    if (argc > 2) {
        std::cerr << "错误: 参数过多" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // 如果只有一个参数（配置文件路径），则设置到全局配置
    if (argc == 2) {
        std::cout << "使用指定的配置文件: " << argv[1] << std::endl;
        // 这里通过函数指针调用SetConfigFilePath，但这是可选的
        // 因为我们将直接使用plugin.h中的函数
    }
    
    // 创建测试数据
    std::cout << "\n===== 创建测试数据 =====" << std::endl;
    
    // 模拟C2S数据（IMAP命令）
    std::string c2sData = "A001 LOGIN user password\r\n";
    c2sData += "A002 SELECT INBOX\r\n";
    c2sData += "A003 FETCH 1 BODY[]\r\n";
    
    // 模拟S2C数据（IMAP响应）
    std::string s2cData = "* OK IMAP server ready\r\n";
    s2cData += "A001 OK LOGIN completed\r\n";
    s2cData += "* 1 EXISTS\r\n";
    s2cData += "* FLAGS (\\Seen \\Draft \\Deleted)\r\n";
    s2cData += "A002 OK SELECT completed\r\n";
    s2cData += "* 1 FETCH (BODY[] {89}\r\n";
    s2cData += "From: sender@example.com\r\nTo: recipient@example.com\r\nSubject: =?UTF-8?B?5Lmd5aSp?=\r\n\r\n测试邮件内容\r\n)\r\n";
    s2cData += "A003 OK FETCH completed\r\n";
    
    // 创建TASK结构体用于C2S测试
    TASK c2sTask;
    memset(&c2sTask, 0, sizeof(TASK));
    
    // 设置C2S任务
    c2sTask.Inform = 0x12;  // 数据传输
    c2sTask.Source.Role = 'C';  // 客户端
    c2sTask.Source.IPvN = 4;  // IPv4
    c2sTask.Source.IPv4 = inet_addr("192.168.0.1");
    c2sTask.Source.Port = 12345;
    c2sTask.Target.Role = 'S';  // 服务器
    c2sTask.Target.IPvN = 4;  // IPv4
    c2sTask.Target.IPv4 = inet_addr("192.168.0.2");
    c2sTask.Target.Port = 143;  // IMAP端口
    c2sTask.Buffer = (unsigned char*)c2sData.c_str();
    c2sTask.Length = c2sData.length();
    
    // 创建TASK结构体用于S2C测试
    TASK s2cTask;
    memset(&s2cTask, 0, sizeof(TASK));
    
    // 设置S2C任务
    s2cTask.Inform = 0x12;  // 数据传输
    s2cTask.Source.Role = 'S';  // 服务器
    s2cTask.Source.IPvN = 4;  // IPv4
    s2cTask.Source.IPv4 = inet_addr("192.168.0.2");
    s2cTask.Source.Port = 143;  // IMAP端口
    s2cTask.Target.Role = 'C';  // 客户端
    s2cTask.Target.IPvN = 4;  // IPv4
    s2cTask.Target.IPv4 = inet_addr("192.168.0.1");
    s2cTask.Target.Port = 12345;
    s2cTask.Buffer = (unsigned char*)s2cData.c_str();
    s2cTask.Length = s2cData.length();
    
    // 调用测试接口 - 使用直接引入的函数符号
    std::cout << "\n===== 调用插件接口 =====" << std::endl;
    
    // 1. 全局初始化
    std::cout << "\n1. 调用 GlobalInit()" << std::endl;
    GlobalInit();
    
    // 2. 线程初始化
    std::cout << "\n2. 调用 ThreadInit()" << std::endl;
    ThreadInit();
    
    // 3. 处理C2S数据包
    std::cout << "\n3. 处理C2S数据包" << std::endl;
    TASK* exportC2STask = nullptr;
    int c2sResult = Filter(&c2sTask, &exportC2STask);
    std::cout << "Filter返回值: " << c2sResult << std::endl;
    
    // 4. 处理S2C数据包
    std::cout << "\n4. 处理S2C数据包" << std::endl;
    TASK* exportS2CTask = nullptr;
    int s2cResult = Filter(&s2cTask, &exportS2CTask);
    std::cout << "Filter返回值: " << s2cResult << std::endl;
    
    // 5. 资源清理
    std::cout << "\n5. 调用 Remove()" << std::endl;
    Remove();
    
    std::cout << "\n===== 测试完成 =====" << std::endl;
    return 0;
}
