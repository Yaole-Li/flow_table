#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>  // 用于IP地址转换函数
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

// 数据过滤(我们主要要干的地方)
int Filter(TASK *Import, TASK **Export) {
    // 置动作为通告
    *Export = Import;

    switch(Import->Inform) {
        case 0X12: {
            // 设置动作为转发
            (*Export)->Action = 0X22;

            // 加处理
            /*
            此处的处理为:
            1. 创建好 HashFlowTable 类
            2. 根据 TASK 中的源端和宿端创建四元组,如果源端的 Role 是 C ,那么创建的四元组是 C2S,否则是 S2C
            3. 根据四元组和 TASK 中的 Buffer 及 Length 创建 InputPacket
            4. 根据 InputPacket 创建 Flow(通过 HashFlowTable 的 processPacket)
            5. 调用对应的 Parser 函数，解析结果会存储在对应的 messages 中
            */

            // 1. 创建好 HashFlowTable 类
            static flow_table::HashFlowTable flowTable;
            
            // 2. 根据 TASK 中的源端和宿端创建四元组
            FourTuple fourTuple;
            
            // 设置源IP和端口
            if (Import->Source.IPvN == 4) {
                fourTuple.srcIPvN = 4;
                fourTuple.srcIPv4 = Import->Source.IPv4;
            } else if (Import->Source.IPvN == 6) {
                fourTuple.srcIPvN = 6;
                memcpy(fourTuple.srcIPv6, Import->Source.IPv6, 16);
            }
            fourTuple.sourcePort = Import->Source.Port;
            
            // 设置目标IP和端口
            if (Import->Target.IPvN == 4) {
                fourTuple.dstIPvN = 4;
                fourTuple.dstIPv4 = Import->Target.IPv4;
            } else if (Import->Target.IPvN == 6) {
                fourTuple.dstIPvN = 6;
                memcpy(fourTuple.dstIPv6, Import->Target.IPv6, 16);
            }
            fourTuple.destPort = Import->Target.Port;
            
            // 3. 创建InputPacket，只包含数据、类型和四元组
            flow_table::InputPacket packet;
            
            // 设置数据包的有效载荷
            std::string payload(reinterpret_cast<char*>(Import->Buffer), Import->Length);
            packet.payload = payload;
            
            // 设置数据包类型（即源角色）
            packet.type = (Import->Source.Role == 'C') ? "C2S" : "S2C";
            
            // 上面已经创建好了四元组
            packet.fourTuple = fourTuple;
            
            // 4. 根据 InputPacket 创建 Flow(通过 HashFlowTable 的 processPacket)
            bool processed = flowTable.processPacket(packet);
            
            // 5. 流内部会自动调用对应的 Parser 函数，解析结果会存储在对应的 messages 中
            if (processed) {
                // 输出所有已解析的消息
                flowTable.outputResults();
            }

            break;
        }
        default:
            (*Export)->Action = 0X21;
            break;
    }

    return 0;
}

int main() {
    TASK task;
    task.Inform = 0x12;
    Filter(&task, nullptr);
    return 0;
}