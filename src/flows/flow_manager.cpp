#include "../../include/flows/flow_manager.h"
#include <iostream>
#include <arpa/inet.h>
#include <set>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <list>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <utility>
#include <sstream>

namespace flow_table {

//-------------------- Flow 类实现 --------------------

Flow::Flow(const FourTuple& c2sTuple, const FourTuple& s2cTuple)
    : c2sTuple(c2sTuple), 
      s2cTuple(s2cTuple),
      c2sBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
      s2cBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
      lastActivityTime(std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count()) { // 初始化最后活动时间为当前时间（毫秒）
    // 初始化流对象
    // TODO: 实现初始化逻辑
}

Flow::Flow(const FourTuple& c2sTuple) 
    : c2sTuple(c2sTuple),
      c2sBuffer(1024 * 1024 * 10), // 默认10MB大小，以后可通过配置文件调整
      s2cBuffer(1024 * 1024 * 10), // 默认10MB大小，以后可通过配置文件调整
      lastActivityTime(std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count()) { // 初始化最后活动时间为当前时间（毫秒）
    
    // 自动生成S2C方向的四元组（反转C2S四元组）
    s2cTuple.srcIPvN = c2sTuple.dstIPvN;
    s2cTuple.dstIPvN = c2sTuple.srcIPvN;
    
    // 复制IPv4或IPv6地址
    if (c2sTuple.srcIPvN == 4) {
        s2cTuple.srcIPv4 = c2sTuple.dstIPv4;
        s2cTuple.dstIPv4 = c2sTuple.srcIPv4;
    } else if (c2sTuple.srcIPvN == 6) {
        memcpy(s2cTuple.srcIPv6, c2sTuple.dstIPv6, 16);
        memcpy(s2cTuple.dstIPv6, c2sTuple.srcIPv6, 16);
    }
    
    // 交换端口
    s2cTuple.sourcePort = c2sTuple.destPort;
    s2cTuple.destPort = c2sTuple.sourcePort;
    
    // 初始化流对象
    // TODO: 实现初始化逻辑
}

void Flow::addC2SData(const std::string& data) {
    // 向C2S缓冲区添加数据
    if (!data.empty()) {
        c2sBuffer.push_back(data);
        std::cout << "添加C2S数据: " << data.size() << " 字节" << std::endl;
    }
    updateLastActivityTime();
}

void Flow::addS2CData(const std::string& data) {
    // 向S2C缓冲区添加数据
    if (!data.empty()) {
        s2cBuffer.push_back(data);
        std::cout << "添加S2C数据: " << data.size() << " 字节" << std::endl;
    }
    updateLastActivityTime();
}

bool Flow::parseC2SData() {
    // 解析C2S缓冲区中的数据
    // 1. 查找完整的IMAP命令（通常以\r\n结尾）
    // 2. 解析命令格式
    // 3. 更新C2S状态
    // 4. 生成Message对象并添加到c2sMessages中

    // TODO: 实现解析逻辑
    Message current_message;    // 存储当前请求信息的结构体，如果解析正常结束则将其添加到c2sMessages尾端
    std::string temp;           // 解析参数时可能会出现多个参数，用于暂存一个参数
    size_t index = 0;              // 下一个要从line中读取的字符的位置
    int left_bracket_count = 0;         // 用于解析参数时的括号匹配
    char curren_char = c2sBuffer.at(index++);    // 当前正在被解析的字符
    size_t end_line_index;

    try {
        size_t buffer_size = c2sBuffer.size();
        if (buffer_size == 0) {
            std::cout << "缓冲区为空" << std::endl;
            return false;
        }
        // 找回车换行
        end_line_index = c2sBuffer.find(0, buffer_size, '\r');
        if (end_line_index == (size_t)(-1)) {
            std::cout << "缓冲区内未找到换行符" << std::endl;
            return false;
        }
        try {
            c2sBuffer.at(end_line_index + 1);
        }
        catch (std::out_of_range e) {
            std::cout << "缓冲区的结尾是\\r" << std::endl;
            return false;
        }
        if (c2sBuffer.at(end_line_index + 1) != '\n') {
            end_line_index--;
            throw std::runtime_error("回车换行不匹配");
        }

        // 解析标签的第一个字节
        if (curren_char >= 33 && curren_char <= 126 && curren_char != '+' && curren_char != '*') {
            current_message.tag.push_back(curren_char);
            curren_char = c2sBuffer.at(index++);
            //解析标签的剩余字节
            while (curren_char >= 33 && curren_char <= 126) {
                current_message.tag.push_back(curren_char);
                curren_char = c2sBuffer.at(index++);
            }
        }
        else {
            throw std::runtime_error("Tag首字符不合法");
        }
        //删除标签和命令之间的空白符（空格和水平制表符）
        while (curren_char == ' ' || curren_char == 9) {
            curren_char = c2sBuffer.at(index++);
        }
        // 解析命令的第一个字节
        if (curren_char >= 33 && curren_char <= 126) {
            current_message.command.push_back(curren_char);
            curren_char = c2sBuffer.at(index++);
            //解析命令的剩余字节
            while (curren_char >= 33 && curren_char <= 126) {
                current_message.command.push_back(curren_char);
                curren_char = c2sBuffer.at(index++);
            }
        }
        else {
            throw std::runtime_error("命令格式不合法");
        }
        //删除命令和参数之间的空白符（空格和水平制表符）
        while (curren_char == ' ' || curren_char == 9) {
            curren_char = c2sBuffer.at(index++);
        }
        //解析参数，直到遇见换行符
        while (curren_char != '\r') {
            temp.clear();
            left_bracket_count = 0;
            // 解析参数的第一个字节
            if (curren_char >= 33 && curren_char <= 126) {
                if (curren_char == '(') {
                    left_bracket_count++;
                }
                temp.push_back(curren_char);
                curren_char = c2sBuffer.at(index++);
            }
            else {
                throw std::runtime_error("参数中发现不可打印字符");
            }
            //解析参数的剩余字节
            while (curren_char >= 32 && curren_char <= 126) {
                if (left_bracket_count == 0) {
                    if (curren_char == ' ') {
                        break;  //跳出while循环
                    }
                    else {
                        if (curren_char == '(') {
                            left_bracket_count ++;
                        }
                        temp.push_back(curren_char);
                        curren_char = c2sBuffer.at(index++);
                    }
                }
                else {
                    if (curren_char == '(') {
                        left_bracket_count++;
                    }
                    else if (curren_char == ')') {
                        left_bracket_count--;
                    }
                    temp.push_back(curren_char);
                    curren_char = c2sBuffer.at(index++);
                }
            }
            current_message.args.push_back(temp);   //保存参数
            //删除直到下一个参数之间的空白符（空格和水平制表符）
            while (curren_char == ' ' || curren_char == 9) {
                curren_char = c2sBuffer.at(index++);
            }
        }
        curren_char = c2sBuffer.at(index++);
        //最后把解析的Message放入c2sMessages末尾
        c2sMessages.push_back(current_message);
        c2sBuffer.erase_up_to(end_line_index + 1);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << ":";   //打印错误信息
        //打印原请求，不可打印字符用16进制ascii码显示
        for (size_t i = 0; i < end_line_index; i++) {
            if (c2sBuffer.at(i) >= 32 && c2sBuffer.at(i) <= 126) {
                std::cerr << c2sBuffer.at(i);
            }
            else {
                std::cerr << "\\0x" << std::setw(2) << std::setfill('0') << std::hex << int(c2sBuffer.at(i));
            }
        }
        std::cerr << std::endl << std::dec;
        c2sBuffer.erase_up_to(end_line_index + 1);
        return false;
    }
    
    // 检查当前解析的消息是否是LOGOUT命令
    if (!c2sMessages.empty()) {
        const auto& message = c2sMessages.back(); // 获取最新添加的消息
        // 不区分大小写比较command字段是否为"logout"
        std::string command = message.command;
        std::transform(command.begin(), command.end(), command.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        
        if (command == "logout") {
            // 发现LOGOUT命令，返回特殊标志，表示需要删除流
            std::cout << "检测到LOGOUT命令" << std::endl;
            return true; // 返回true表示检测到LOGOUT命令
        }
    }
    
    return false; // 没有检测到LOGOUT命令
}

/*
不在这里实现了,在s2c_parser.cpp中实现
bool Flow::parseS2CData() {
    // 解析S2C缓冲区中的数据
    // 1. 查找完整的IMAP响应
    // 2. 解析响应内容
    // 3. 更新S2C状态
    // 4. 生成Message对象并添加到s2cMessages中
    // 5. 如果是邮件数据，解析并生成Email对象
    // TODO: 实现解析逻辑
    return false; // 暂时返回false
}
*/

void Flow::outputMessages() const {
    // 输出流中的消息数据
    
    // 1. 输出C2S方向的消息
    std::cout << "\n===== C2S 方向消息 (" << c2sMessages.size() << " 条) =====" << std::endl;
    for (size_t i = 0; i < c2sMessages.size(); ++i) {
        const Message& msg = c2sMessages[i];
        std::cout << "[" << i + 1 << "] 标签: " << msg.tag << ", 命令: " << msg.command;
        
        // 输出参数
        if (!msg.args.empty()) {
            std::cout << ", 参数: ";
            for (size_t j = 0; j < msg.args.size(); ++j) {
                std::cout << msg.args[j];
                if (j < msg.args.size() - 1) {
                    std::cout << ", ";
                }
            }
        }
        
        // 输出邮件信息（如果有）
        if (!msg.fetch.empty()) {
            std::cout << ", 获取到 " << msg.fetch.size() << " 封邮件";
        }
        
        std::cout << std::endl;
    }
    
    // 2. 输出S2C方向的消息
    std::cout << "\n===== S2C 方向消息 (" << s2cMessages.size() << " 条) =====" << std::endl;
    for (size_t i = 0; i < s2cMessages.size(); ++i) {
        const Message& msg = s2cMessages[i];
        std::cout << "\n[" << i + 1 << "] ";
        
        // 输出标签（如果有）
        if (!msg.tag.empty()) {
            std::cout << "标签: " << msg.tag << ", ";
        }
        
        // 输出命令（如果有）
        if (!msg.command.empty()) {
            std::cout << "命令: " << msg.command;
        }
        
        // 输出参数
        if (!msg.args.empty()) {
            std::cout << ", 参数: ";
            for (size_t j = 0; j < msg.args.size(); ++j) {
                std::cout << msg.args[j];
                if (j < msg.args.size() - 1) {
                    std::cout << ", ";
                }
            }
        }
        
        std::cout << std::endl;
        
        // 输出邮件信息（如果有）
        if (!msg.fetch.empty()) {
            std::cout << "  └─ 包含 " << msg.fetch.size() << " 封邮件" << std::endl;
            
            // 详细输出每封邮件的信息
            for (size_t j = 0; j < msg.fetch.size(); ++j) {
                const Email& email = msg.fetch[j];
                std::cout << "     ┌─ 邮件 #" << j + 1 << std::endl;
                
                // 输出基本信息
                if (email.sequence_number > 0) {
                    std::cout << "     │  序列号: " << email.sequence_number << std::endl;
                }
                if (email.uid > 0) {
                    std::cout << "     │  UID: " << email.uid << std::endl;
                }
                if (email.rfc822_size > 0) {
                    std::cout << "     │  大小: " << email.rfc822_size << " 字节" << std::endl;
                }
                if (!email.flags.empty()) {
                    std::cout << "     │  标志: " << email.flags << std::endl;
                }
                if (!email.internaldate.empty()) {
                    std::cout << "     │  内部日期: " << email.internaldate << std::endl;
                }
                
                // 输出头部信息
                std::cout << "     │" << std::endl;
                std::cout << "     ├─ 邮件头部" << std::endl;
                
                if (!email.body.header.from.empty()) {
                    std::cout << "     │  发件人: " << email.body.header.from << std::endl;
                }
                
                if (!email.body.header.to.empty()) {
                    std::cout << "     │  收件人: ";
                    for (size_t k = 0; k < email.body.header.to.size(); ++k) {
                        std::cout << email.body.header.to[k];
                        if (k < email.body.header.to.size() - 1) {
                            std::cout << ", ";
                        }
                    }
                    std::cout << std::endl;
                }
                
                if (!email.body.header.cc.empty()) {
                    std::cout << "     │  抄送: ";
                    for (size_t k = 0; k < email.body.header.cc.size(); ++k) {
                        std::cout << email.body.header.cc[k];
                        if (k < email.body.header.cc.size() - 1) {
                            std::cout << ", ";
                        }
                    }
                    std::cout << std::endl;
                }
                
                if (!email.body.header.subject.empty()) {
                    std::cout << "     │  主题: ";
                    for (size_t k = 0; k < email.body.header.subject.size(); ++k) {
                        std::cout << email.body.header.subject[k];
                        if (k < email.body.header.subject.size() - 1) {
                            std::cout << " ";
                        }
                    }
                    std::cout << std::endl;
                }
                
                if (!email.body.header.date.empty()) {
                    std::cout << "     │  日期: " << email.body.header.date << std::endl;
                }
                
                if (!email.body.header.message_id.empty()) {
                    std::cout << "     │  消息ID: " << email.body.header.message_id[0] << std::endl;
                }
                
                // 输出正文信息
                if (!email.body.text.empty()) {
                    std::cout << "     │" << std::endl;
                    std::cout << "     ├─ 邮件正文" << std::endl;
                    
                    // 将正文按行分割并添加缩进
                    std::istringstream iss(email.body.text);
                    std::string line;
                    bool firstLine = true;
                    while (std::getline(iss, line)) {
                        if (line.empty()) continue;
                        
                        if (firstLine) {
                            std::cout << "     │  " << line << std::endl;
                            firstLine = false;
                        } else {
                            std::cout << "     │  " << line << std::endl;
                        }
                    }
                }
                
                // 输出其他信息
                if (!email.envelope.empty() || !email.bodystructure.empty()) {
                    std::cout << "     │" << std::endl;
                    std::cout << "     ├─ 其他信息" << std::endl;
                    
                    if (!email.envelope.empty()) {
                        std::cout << "     │  信封: " << email.envelope << std::endl;
                    }
                    
                    if (!email.bodystructure.empty()) {
                        std::cout << "     │  正文结构: " << email.bodystructure << std::endl;
                    }
                }
                
                std::cout << "     └────────────────────────────────────" << std::endl;
            }
        }
    }
}

void Flow::cleanup() {
    // 清理流对象的资源
    c2sMessages.clear();
    s2cMessages.clear();
    c2sState.clear();
    s2cState.clear();
    // 清空缓冲区 - 使用erase_up_to方法删除所有内容
    if (c2sBuffer.size() > 0) {
        c2sBuffer.erase_up_to(c2sBuffer.size() - 1);
    }
    if (s2cBuffer.size() > 0) {
        s2cBuffer.erase_up_to(s2cBuffer.size() - 1);
    }
}

void Flow::updateLastActivityTime() {
    // 更新最后活动时间为当前时间（毫秒）
    lastActivityTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t Flow::getLastActivityTime() const {
    // 返回最后活动时间（毫秒）
    return lastActivityTime;
}

bool Flow::isTimeout(int64_t timeoutMilliseconds) const {
    // 获取当前时间（毫秒）
    int64_t currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // 检查是否超时
    return (currentTimeMs - lastActivityTime) > timeoutMilliseconds;
}

//-------------------- HashFlowTable 类实现 --------------------

/**
 * 将字符串格式的IP地址转换为四元组中的IP表示(但是现在不用了)
 * @param ipStr IP地址字符串
 * @param tuple 目标四元组
 * @param isSource 是否为源IP
 * @return 转换是否成功
 */
bool convertStringToIP(const std::string& ipStr, FourTuple& tuple, bool isSource) {
    // 尝试解析为IPv4地址
    struct sockaddr_in sa4;
    if (inet_pton(AF_INET, ipStr.c_str(), &(sa4.sin_addr)) == 1) {
        // 成功解析为IPv4
        if (isSource) {
            tuple.srcIPvN = 4;
            tuple.srcIPv4 = ntohl(sa4.sin_addr.s_addr);
        } else {
            tuple.dstIPvN = 4;
            tuple.dstIPv4 = ntohl(sa4.sin_addr.s_addr);
        }
        return true;
    }
    
    // 尝试解析为IPv6地址
    struct sockaddr_in6 sa6;
    if (inet_pton(AF_INET6, ipStr.c_str(), &(sa6.sin6_addr)) == 1) {
        // 成功解析为IPv6
        if (isSource) {
            tuple.srcIPvN = 6;
            memcpy(tuple.srcIPv6, sa6.sin6_addr.s6_addr, 16);
        } else {
            tuple.dstIPvN = 6;
            memcpy(tuple.dstIPv6, sa6.sin6_addr.s6_addr, 16);
        }
        return true;
    }
    
    // 解析失败
    return false;
}

HashFlowTable::HashFlowTable() : flowTimeoutMilliseconds(120000) {
    // 初始化哈希流表
    // 默认超时时间为120000毫秒（2分钟）
}

HashFlowTable::~HashFlowTable() {
    // 创建一个集合来跟踪已处理的流指针，避免重复删除
    std::set<Flow*> processedFlows;
    
    std::cout << "\n===== 析构函数开始执行 =====" << std::endl;
    
    // 收集所有唯一的流对象
    std::vector<Flow*> uniqueFlows;
    for (auto& pair : flowMap) {
        Flow* flow = pair.second;
        if (flow && processedFlows.find(flow) == processedFlows.end()) {
            processedFlows.insert(flow);
            uniqueFlows.push_back(flow);
        }
    }
    
    std::cout << "准备清理 " << uniqueFlows.size() << " 个流对象" << std::endl;
    
    // 先清空映射，防止后续操作引用已删除的对象
    flowMap.clear();
    timeOrderedFlows.clear();
    
    // 清空时间桶
    timeBuckets.clear();
    
    // 删除所有流对象
    int flowCount = 0;
    for (Flow* flow : uniqueFlows) {
        flowCount++;
        std::cout << "正在清理流 #" << flowCount << ": ";
        
        // 打印四元组信息
        /*
        const FourTuple& tuple = flow->getC2STuple();
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
        */
        
        std::cout << "  - 调用流的cleanup()方法" << std::endl;
        flow->cleanup();
        
        std::cout << "  - 删除流对象" << std::endl;
        delete flow;
    }
    
    std::cout << "所有流已清理完毕" << std::endl;
    std::cout << "===== 析构函数执行完成 =====" << std::endl;
}

void HashFlowTable::setFlowTimeout(int64_t milliseconds) {
    // 设置流超时时间（毫秒）
    flowTimeoutMilliseconds = milliseconds;
}

void HashFlowTable::addToTimeBucket(Flow* flow, int64_t timestampMs) {
    if (!flow) return;
    
    // 计算桶的时间戳（向下取整到最近的BUCKET_INTERVAL）
    int64_t bucketTimeMs = timestampMs - (timestampMs % BUCKET_INTERVAL);
    
    // 添加到对应的时间桶
    timeBuckets[bucketTimeMs].insert(flow);
}

void HashFlowTable::removeFromTimeBucket(Flow* flow, int64_t timestampMs) {
    if (!flow) return;
    
    // 计算桶的时间戳
    int64_t bucketTimeMs = timestampMs - (timestampMs % BUCKET_INTERVAL);
    
    // 使用键直接查找和操作，避免迭代器问题
    if (timeBuckets.count(bucketTimeMs) > 0) {
        TimeBucket& bucket = timeBuckets[bucketTimeMs];
        bucket.erase(flow);
        
        // 如果桶为空，移除桶
        if (bucket.empty()) {
            timeBuckets.erase(bucketTimeMs);
        }
    }
}

void HashFlowTable::moveToNewTimeBucket(Flow* flow, int64_t oldTimestampMs, int64_t newTimestampMs) {
    if (!flow) return;
    
    // 计算旧桶和新桶的时间戳
    int64_t oldBucketTimeMs = oldTimestampMs - (oldTimestampMs % BUCKET_INTERVAL);
    int64_t newBucketTimeMs = newTimestampMs - (newTimestampMs % BUCKET_INTERVAL);
    
    // 如果桶不同，则移动
    if (oldBucketTimeMs != newBucketTimeMs) {
        removeFromTimeBucket(flow, oldTimestampMs);
        addToTimeBucket(flow, newTimestampMs);
    }
}

void HashFlowTable::addToTimeOrderedList(Flow* flow) {
    // 将流添加到时间排序的链表末尾（最新）
    if (flow) {
        timeOrderedFlows.push_back(flow);
        
        // 添加到时间桶中
        int64_t timestampMs = flow->getLastActivityTime();
        addToTimeBucket(flow, timestampMs);
    }
}

void HashFlowTable::removeFromTimeOrderedList(Flow* flow) {
    // 从时间排序链表中移除流
    if (flow) {
        timeOrderedFlows.remove(flow);
        
        // 从时间桶中移除
        int64_t timestampMs = flow->getLastActivityTime();
        removeFromTimeBucket(flow, timestampMs);
    }
}

void HashFlowTable::updateFlowPosition(Flow* flow) {
    if (!flow) return;
    
    // 保存旧的时间戳
    int64_t oldTimestampMs = flow->getLastActivityTime();
    
    // 从链表中移除
    removeFromTimeOrderedList(flow);
    // 再添加到链表末尾（最新）
    addToTimeOrderedList(flow);
    
    // 更新时间桶（在addToTimeOrderedList中已经添加到新桶）
    // 这里不需要额外操作，因为removeFromTimeOrderedList已经从旧桶移除
}

void HashFlowTable::checkAndCleanupTimeoutFlows() {
    // 获取当前时间（毫秒）
    int64_t currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // 创建一个临时向量存储要删除的流
    std::vector<Flow*> flowsToDelete;
    
    // 计算超时时间点
    int64_t timeoutBeforeMs = currentTimeMs - flowTimeoutMilliseconds;
    
    // 首先找出所有需要检查的时间桶键
    std::vector<int64_t> bucketsToCheck;
    for (const auto& pair : timeBuckets) {
        if (pair.first <= timeoutBeforeMs) {
            bucketsToCheck.push_back(pair.first);
        }
    } 
    
    // 现在处理这些桶
    for (int64_t bucketTimeMs : bucketsToCheck) {
        // 获取桶（如果还存在）
        if (timeBuckets.count(bucketTimeMs) > 0) {
            // 创建要检查的流的副本，避免在遍历过程中修改
            std::vector<Flow*> flowsToCheck;
            for (Flow* flow : timeBuckets[bucketTimeMs]) {
                flowsToCheck.push_back(flow);
            }
            
            // 检查每个流
            for (Flow* flow : flowsToCheck) {
                if (flow->isTimeout(flowTimeoutMilliseconds)) {
                    // 将超时流添加到待删除列表
                    flowsToDelete.push_back(flow);
                    // 从桶中移除
                    timeBuckets[bucketTimeMs].erase(flow);
                }
            }
            
            // 如果桶为空，移除桶
            if (timeBuckets[bucketTimeMs].empty()) {
                timeBuckets.erase(bucketTimeMs);
            }
        }
    }
    
    // 删除超时的流
    for (Flow* flow : flowsToDelete) {
        // 从时间排序链表中移除
        timeOrderedFlows.remove(flow);
        // 删除流
        deleteFlow(flow);
    }
}

Flow* HashFlowTable::getOrCreateFlow(const FourTuple& fourTuple) {
    // 计算四元组的哈希值
    int hash = hashFourTuple(fourTuple);
    
    // 查找是否已存在对应的流
    auto range = flowMap.equal_range(hash);
    for (auto it = range.first; it != range.second; ++it) {
        Flow* flow = it->second;
        if (flow && flow->getC2STuple() == fourTuple) {
            // 找到匹配的流，更新其最后活动时间
            flow->updateLastActivityTime();
            // 更新流在时间链表中的位置
            updateFlowPosition(flow);
            return flow;
        }
    }
    
    // 如果未找到匹配的流，创建新流
    std::cout << "未找到匹配的流，创建新流" << std::endl;
    
    // 创建新流 - fourTuple是C2S方向
    Flow* newFlow = new Flow(fourTuple);
    
    // 存储流，直接使用原始哈希值
    flowMap.insert(std::pair<int, Flow*>(hash, newFlow));
    
    // 将新流添加到时间排序的链表中
    addToTimeOrderedList(newFlow);
    
    // 注释掉这行代码，避免递归调用和多线程冲突
    // checkAndCleanupTimeoutFlows();
    
    return newFlow;
}

void HashFlowTable::deleteFlow(Flow* flow) {
    if (!flow) return;
    
    // 在unordered_multimap中查找并删除特定流对象
    // 我们需要遍历所有元素，找到值等于flow的键值对
    using FlowMapIterator = std::unordered_multimap<int, Flow*>::iterator;
    std::vector<std::pair<FlowMapIterator, FlowMapIterator>> rangesToRemove;
    
    // 首先找出所有包含此流的键
    std::vector<int> keysWithFlow;
    for (const auto& pair : flowMap) {
        if (pair.second == flow) {
            keysWithFlow.push_back(pair.first);
        }
    }
    
    // 对于每个键，找到对应的流并删除
    for (int key : keysWithFlow) {
        auto range = flowMap.equal_range(key);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == flow) {
                // 找到了要删除的流，从flowMap中移除
                flowMap.erase(it);
                break; // 找到并删除后跳出内层循环
            }
        }
    }
    
    // 确保从时间链表中移除
    removeFromTimeOrderedList(flow);
    
    // 调用流的清理方法
    flow->cleanup();
    
    // 删除流对象
    delete flow;
}

bool HashFlowTable::processPacket(const InputPacket& packet) {
    // 获取或创建对应的流
    // 注意：此时packet.fourTuple已经被规范化为C2S方向
    Flow* flow = getOrCreateFlow(packet.fourTuple);
    if (!flow) {
        std::cerr << "创建流失败" << std::endl;
        return false;
    }
    
    // 根据数据包类型添加数据并进行解析
    bool needDeleteFlow = false;
    
    if (packet.type == "C2S") {
        flow->addC2SData(packet.payload);
        // 如果parseC2SData返回true，表示检测到LOGOUT命令
        needDeleteFlow = flow->parseC2SData();
    } else if (packet.type == "S2C") {
        flow->addS2CData(packet.payload);
        flow->parseS2CData();
    } else {
        std::cerr << "未知的数据包类型" << std::endl;
        return false;
    }
    
    // 如果检测到LOGOUT命令，直接删除流
    if (needDeleteFlow) {
        std::cout << "执行流删除（LOGOUT命令）" << std::endl;
        deleteFlow(flow);
        return true;
    }
    
    return true;
}

size_t HashFlowTable::getTotalFlows() const {
    // 获取流总数
    return flowMap.size();
}

void HashFlowTable::outputResults() const {
    // 输出所有流的处理结果
    static std::set<Flow*> processedFlows;
    for (const auto& pair : flowMap) {
        // 确保每个流只输出一次（因为同一个流可能有两个哈希键）
        if (processedFlows.find(pair.second) == processedFlows.end()) {
            pair.second->outputMessages();
            processedFlows.insert(pair.second);
        }
    }
    // 清空已处理流的集合，为下一次调用做准备
    processedFlows.clear();
}

std::vector<Flow*> HashFlowTable::getAllFlows() {
    std::vector<Flow*> result;
    std::set<Flow*> uniqueFlows;
    
    // 遍历所有流，确保每个流只添加一次
    for (const auto& pair : flowMap) {
        if (uniqueFlows.find(pair.second) == uniqueFlows.end()) {
            result.push_back(pair.second);
            uniqueFlows.insert(pair.second);
        }
    }
    
    return result;
}

int HashFlowTable::hashFourTuple(const FourTuple& fourTuple) const {
    std::size_t hash = 0;
    
    // 哈希IP地址
    if (fourTuple.srcIPvN == 4) {
        hash = std::hash<uint32_t>{}(fourTuple.srcIPv4);
        hash ^= std::hash<uint32_t>{}(fourTuple.dstIPv4) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    } else if (fourTuple.srcIPvN == 6) {
        for (int i = 0; i < 16; i++) {
            hash ^= std::hash<unsigned char>{}(fourTuple.srcIPv6[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<unsigned char>{}(fourTuple.dstIPv6[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
    }
    
    // 哈希端口
    hash ^= std::hash<int>{}(fourTuple.sourcePort) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= std::hash<int>{}(fourTuple.destPort) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    return static_cast<int>(hash);
}

} // namespace flow_table
