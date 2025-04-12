#include "../../include/flows/flow_manager.h"
#include <iostream>
#include <arpa/inet.h>
#include <set>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <list>
#include <map>

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
      c2sBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
      s2cBuffer(1024 * 1024), // 默认1MB大小，可通过配置文件调整
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
    // TODO: 实现数据添加逻辑
    updateLastActivityTime();
}

void Flow::addS2CData(const std::string& data) {
    // 向S2C缓冲区添加数据
    // TODO: 实现数据添加逻辑
    updateLastActivityTime();
}

bool Flow::parseC2SData() {
    // 解析C2S缓冲区中的数据
    // 1. 查找完整的IMAP命令（通常以\r\n结尾）
    // 2. 解析命令格式
    // 3. 更新C2S状态
    // 4. 生成Message对象并添加到c2sMessages中
    // TODO: 实现解析逻辑
    
    // 检查是否有LOGOUT命令
    for (const auto& message : c2sMessages) {
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

bool Flow::isTimeout(size_t timeoutMilliseconds) const {
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
    // 释放所有流对象
    // 使用getAllFlows获取所有唯一的流对象
    std::vector<Flow*> allFlows = getAllFlows();
    
    // 使用deleteFlow方法删除每个流
    for (Flow* flow : allFlows) {
        // 不使用时间链表排序的deleteFlow版本
        // 因为我们将删除所有流，不需要维护链表结构
        
        // 查找所有指向这个流的映射
        std::vector<int> keysToRemove;
        for (auto& pair : flowMap) {
            if (pair.second == flow) {
                keysToRemove.push_back(pair.first);
            }
        }
        
        // 从映射中删除所有引用
        for (int key : keysToRemove) {
            flowMap.erase(key);
        }
        
        // 调用流的清理方法
        flow->cleanup();
        
        // 删除流对象
        delete flow;
    }
    
    // 清空所有集合
    flowMap.clear();
    timeOrderedFlows.clear();
}

void HashFlowTable::setFlowTimeout(size_t milliseconds) {
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
    
    // 在所有流中查找匹配的四元组
    // 这里需要遍历所有流，因为可能存在哈希冲突
    // 我们需要找到四元组完全匹配的流，而不仅仅是哈希值匹配的流
    for (const auto& pair : flowMap) {
        Flow* flow = pair.second;
        if (flow->getC2STuple() == fourTuple) {
            // 找到了完全匹配四元组的流
            flow->updateLastActivityTime();
            // 更新流在时间排序链表中的位置
            updateFlowPosition(flow);
            return flow;
        }
    }
    
    // 如果未找到匹配的流，创建新流
    std::cout << "未找到匹配的流，创建新流" << std::endl;
    
    // 查找哈希值（处理可能的哈希冲突）
    int newHash = hash;
    while (flowMap.find(newHash) != flowMap.end()) {
        newHash++;
    }
    
    // 创建新流 - fourTuple是C2S方向
    Flow* newFlow = new Flow(fourTuple);
    
    // 存储流，使用可用的哈希值
    flowMap[newHash] = newFlow;
    
    // 将新流添加到时间排序的链表中
    addToTimeOrderedList(newFlow);

    // 检查并清理超时的流
    checkAndCleanupTimeoutFlows();
    
    return newFlow;
}

void HashFlowTable::deleteFlow(Flow* flow) {
    if (!flow) return;
    
    // 查找所有指向这个流的映射
    std::vector<int> keysToRemove;
    for (auto& pair : flowMap) {
        if (pair.second == flow) {
            keysToRemove.push_back(pair.first);
        }
    }
    
    // 从映射中删除所有引用
    for (int key : keysToRemove) {
        flowMap.erase(key);
    }
    
    // 确保从时间链表中移除
    removeFromTimeOrderedList(flow);
    
    // 调用流的清理方法
    flow->cleanup();
    
    // 输出调试信息
    std::cout << "删除流" << std::endl;
    
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
