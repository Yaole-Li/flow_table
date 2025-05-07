/**
 * @file plugin.h
 * @brief IMAP流量分析和关键词检测插件接口
 * 
 * 本文件定义了IMAP流量分析和关键词检测插件的四个主要接口：
 * - GlobalInit：全局初始化
 * - ThreadInit：线程初始化
 * - Filter：数据包过滤处理
 * - Remove：资源清理
 * 
 * 这些接口函数可以被其他项目调用，实现对IMAP流量的解析和关键词检测。
 */

#ifndef FLOW_TABLE_PLUGIN_H
#define FLOW_TABLE_PLUGIN_H

// DLL导出宏定义
#ifdef _WIN32
#define DLL_PUBLIC __declspec(dllexport)
#else
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#endif

// 定义BOOK类型 - 用于插件内部数据存储
typedef struct _BOOK {
    void* data;  // 通用数据指针
} BOOK;

#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include "../flows/flow_manager.h"
#include "../tools/CircularString.h"
#include "../tools/types.h"
#include "../config/config_parser.h"
#include "../../extension/auto_AC/include/AhoCorasick.h"

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
extern "C" BOOK g_Book;      // 全局BOOK
extern "C" BOOK *WorkBook;   // 线程BOOK

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------ 导出接口函数 ------------------------------

/**
 * @brief 插件创建函数 (全局初始化)
 * 
 * 该函数在程序启动时执行一次，用于初始化全局资源
 * 
 * @param Version 插件版本
 * @param Amount 参数数量
 * @param Option 选项参数
 * @return 初始化结果，0表示成功
 */
DLL_PUBLIC int Create(unsigned short Version, unsigned short Amount, const char *Option);

/**
 * @brief 插件构建函数 (线程初始化)
 * 
 * 该函数在每个工作线程启动时执行，用于初始化线程级别资源
 * 
 * @param Thread 线程编号
 * @param Option 选项参数
 * @return 初始化结果，0表示成功
 */
DLL_PUBLIC int Single(unsigned short Thread, const char *Option);

/**
 * @brief 数据过滤函数
 * 
 * 处理每个数据包，解析IMAP协议内容并进行关键词检测
 * 
 * @param Import 输入的数据包任务
 * @param Export 输出的数据包任务
 * @return 处理结果，0表示成功
 */
DLL_PUBLIC int Filter(TASK *Import, TASK **Export);

/**
 * @brief 插件拆除函数 (资源清理)
 * 
 * 负责释放所有资源
 */
DLL_PUBLIC void Remove();

// 设置配置文件路径函数
/**
 * @brief 设置配置文件路径
 * 
 * @param path 配置文件路径
 */
DLL_PUBLIC void SetConfigFilePath(const char* path);

#ifdef __cplusplus
}
#endif

#endif // FLOW_TABLE_PLUGIN_H
