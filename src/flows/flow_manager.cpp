#include "../../include/flows/flow_manager.h"
#include <iostream>
#include <arpa/inet.h>  // 用于IP地址转换函数

namespace flow_table {

//-------------------- Flow 类实现 --------------------

Flow::Flow(const FourTuple& c2sTuple, const FourTuple& s2cTuple)
    : c2sTuple(c2sTuple), 
      s2cTuple(s2cTuple),
      c2sBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
      s2cBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
      markedForDeletion(false) {
    // 初始化流对象
    // TODO: 实现初始化逻辑
}

void Flow::addC2SData(const std::string& data) {
    // 向C2S缓冲区添加数据
    // TODO: 实现数据添加逻辑
}

void Flow::addS2CData(const std::string& data) {
    // 向S2C缓冲区添加数据
    // TODO: 实现数据添加逻辑
}

bool Flow::parseC2SData() {
    // 解析C2S缓冲区中的数据
    // 1. 查找完整的IMAP命令（通常以\r\n结尾）
    // 2. 解析命令格式
    // 3. 更新C2S状态
    // 4. 生成Message对象并添加到c2sMessages中
    // TODO: 实现解析逻辑
    return false; // 暂时返回false
}

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

void Flow::outputMessages() const {
    // 输出流中的消息数据
    // 1. 输出C2S方向的消息
    // 2. 输出S2C方向的消息
    // TODO: 实现输出逻辑
}

void Flow::markForDeletion() {
    // 标记流可以删除
    markedForDeletion = true;
}

bool Flow::isMarkedForDeletion() const {
    // 检查流是否标记为删除
    return markedForDeletion;
}

//-------------------- HashFlowTable 类实现 --------------------

/**
 * 将字符串格式的IP地址转换为四元组中的IP表示
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

HashFlowTable::HashFlowTable() {
    // 初始化哈希流表
    // TODO: 实现初始化逻辑
}

HashFlowTable::~HashFlowTable() {
    // 释放所有流对象
    for (auto& pair : flowMap) {
        delete pair.second;
    }
    flowMap.clear();
}

Flow* HashFlowTable::getOrCreateFlow(const FourTuple& fourTuple) {
    // 计算四元组的哈希值
    int hash = hashFourTuple(fourTuple);
    
    // 查找是否已存在对应的流
    auto it = flowMap.find(hash);
    if (it != flowMap.end()) {
        return it->second;
    }
    
    // 创建反向四元组并尝试查找
    FourTuple reverseTuple;
    // 交换源和目的
    reverseTuple.srcIPvN = fourTuple.dstIPvN;
    reverseTuple.dstIPvN = fourTuple.srcIPvN;
    reverseTuple.sourcePort = fourTuple.destPort;
    reverseTuple.destPort = fourTuple.sourcePort;
    
    // 根据IP版本拷贝IP地址
    if (fourTuple.srcIPvN == 4) {
        reverseTuple.srcIPv4 = fourTuple.dstIPv4;
        reverseTuple.dstIPv4 = fourTuple.srcIPv4;
    } else if (fourTuple.srcIPvN == 6) {
        // 对于IPv6，需要逐字节拷贝
        for (int i = 0; i < 16; i++) {
            reverseTuple.srcIPv6[i] = fourTuple.dstIPv6[i];
            reverseTuple.dstIPv6[i] = fourTuple.srcIPv6[i];
        }
    }
    
    // 计算反向四元组的哈希值并查找
    int reverseHash = hashFourTuple(reverseTuple);
    it = flowMap.find(reverseHash);
    if (it != flowMap.end()) {
        // 找到了反向流，直接返回
        return it->second;
    }
    
    // 两个方向都没有找到，创建新流
    // 此处发现个问题,无法保证fourTuple和reverseTuple一定是构造函数中的 C2S 和 S2C 的关系
    // 但是貌似不影响整个流程,因为在processPacket中,会根据 packet->type 来确定调用哪个 Parser
    Flow* newFlow = new Flow(fourTuple, reverseTuple);
    
    // 存储流，我们需要同时用两个哈希值索引相同的流
    // 这样无论是正向还是反向四元组，都能找到同一个流
    flowMap[hash] = newFlow;
    flowMap[reverseHash] = newFlow;
    
    return newFlow;
}

bool HashFlowTable::processPacket(const InputPacket& packet) {
    // 获取或创建对应的流
    Flow* flow = getOrCreateFlow(packet.fourTuple);
    if (!flow) {
        std::cerr << "创建流失败" << std::endl;
        return false;
    }
    
    // 根据数据包方向添加数据
    if (packet.type == "C2S") {
        flow->addC2SData(packet.payload);
        flow->parseC2SData();
    } else if (packet.type == "S2C") {
        flow->addS2CData(packet.payload);
        flow->parseS2CData();
    } else {
        std::cerr << "未知的数据包方向" << std::endl;
        return false;
    }
    
    return true;
}

void HashFlowTable::cleanupMarkedFlows() {
    // 清理标记为删除的流
    // 1. 遍历所有流
    // 2. 删除标记为删除的流
    // TODO: 实现清理逻辑
}

size_t HashFlowTable::getTotalFlows() const {
    // 获取流总数
    return flowMap.size();
}

void HashFlowTable::outputResults() const {
    // 输出所有流的处理结果
    // 1. 遍历所有流
    // 2. 调用每个流的outputMessages方法
    // TODO: 实现输出逻辑
}

int HashFlowTable::hashFourTuple(const FourTuple& fourTuple) const {
    // 计算四元组的哈希值
    int hash = 0;
    
    // 根据IP版本使用不同的哈希计算方法
    if (fourTuple.srcIPvN == 4) {
        // IPv4哈希计算
        hash = 31 * hash + static_cast<int>(fourTuple.srcIPv4);
        hash = 31 * hash + static_cast<int>(fourTuple.dstIPv4);
    } else if (fourTuple.srcIPvN == 6) {
        // IPv6哈希计算
        for (int i = 0; i < 16; i++) {
            hash = 31 * hash + static_cast<int>(fourTuple.srcIPv6[i]);
            hash = 31 * hash + static_cast<int>(fourTuple.dstIPv6[i]);
        }
    }
    
    // 加入端口信息
    hash = 31 * hash + fourTuple.sourcePort;
    hash = 31 * hash + fourTuple.destPort;
    
    return hash;
}

} // namespace flow_table
