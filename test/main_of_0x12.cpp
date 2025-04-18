#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <types.h>
#include "../include/flows/flow_manager.h"
#include "../include/tools/CircularString.h"
#include "../include/tools/types.h"

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

// ------------------------------ 1. 全局初始化 ------------------------------
// 该部分只在程序启动时执行一次
void GlobalInit() {
    std::cout << "执行全局初始化..." << std::endl;
    
    // 暂时想不到要放什么
    // AC 自动机做匹配的
    
    std::cout << "全局初始化完成" << std::endl;
}

// ------------------------------ 2. 单线程初始化 ------------------------------
// 该部分在每个工作线程启动时执行
void ThreadInit() {
    std::cout << "执行线程初始化..." << std::endl;
    
    // 创建哈希流表（每个线程一个实例）
    flowTable = new flow_table::HashFlowTable();
    
    // 设置流超时时间为120秒
    flowTable->setFlowTimeout(120000);
    
    std::cout << "线程初始化完成" << std::endl;
}

// ------------------------------ 3. Filter 处理函数 ------------------------------
// 数据过滤函数，处理每个数据包
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
                flowTable->outputResults();
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
    
    std::cout << "清理完成" << std::endl;
}

// 测试主函数，模拟插件的使用
int main() {
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
    
    // 3. 创建测试数据包
    TASK task;
    task.Inform = 0x12;
    task.Source.Role = 'C';
    task.Source.IPvN = 4;
    task.Source.IPv4 = 0x01020304; // 1.2.3.4
    task.Source.Port = 1234;
    task.Target.Role = 'S';
    task.Target.IPvN = 4;
    task.Target.IPv4 = 0x05060708; // 5.6.7.8
    task.Target.Port = 80;
    
    // 创建测试数据
    const char* testData = "a0004 FETCH 1:13 (FLAGS INTERNALDATE RFC822.SIZE BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT TO CC MESSAGE-ID REFERENCES CONTENT-TYPE IN-REPLY-TO REPLY-TO)])";
    size_t dataLen = strlen(testData);
    unsigned char* buffer = new unsigned char[dataLen];
    memcpy(buffer, testData, dataLen);
    
    task.Buffer = buffer;
    task.Length = dataLen;
    
    // 处理数据包
    Filter(&task, nullptr);
    
    // 释放测试数据
    delete[] buffer;
    
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