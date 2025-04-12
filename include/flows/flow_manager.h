#ifndef FLOW_TABLE_FLOW_MANAGER_H
#define FLOW_TABLE_FLOW_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <list>
#include <ctime>
#include "../tools/types.h"
#include "../tools/CircularString.h"

namespace flow_table {

/**
 * @brief 输入数据包结构体，表示一个网络数据包
 */
struct InputPacket {
    std::string payload;           // 数据包有效载荷
    std::string type;              // 数据包类型(C2S/S2C)，同时表示源角色
    FourTuple fourTuple;           // 网络四元组，包含源和目标的IP和端口信息
};

/**
 * @brief 流类，表示一个网络流，包含双向数据
 */
class Flow {
public:
    /**
     * @brief 构造函数（传统方式，接收两个方向的四元组）
     * @param c2sTuple C2S方向的四元组
     * @param s2cTuple S2C方向的四元组
     */
    Flow(const FourTuple& c2sTuple, const FourTuple& s2cTuple);

    /**
     * @brief 简化构造函数（优化方式，只接收C2S方向的四元组）
     * @param c2sTuple C2S方向的四元组
     */
    Flow(const FourTuple& c2sTuple);

    /**
     * @brief 向C2S缓冲区添加数据
     * @param data 要添加的数据
     */
    void addC2SData(const std::string& data);

    /**
     * @brief 向S2C缓冲区添加数据
     * @param data 要添加的数据
     */
    void addS2CData(const std::string& data);

    /**
     * @brief 解析C2S缓冲区数据
     * @return 是否成功解析
     */
    bool parseC2SData();

    /**
     * @brief 解析S2C缓冲区数据
     * @return 是否成功解析
     */
    bool parseS2CData();

    /**
     * @brief 输出流中的消息数据
     */
    void outputMessages() const;

    /**
     * @brief 清理流对象的资源
     */
    void cleanup();
    
    /**
     * @brief 更新流的最后活动时间为当前时间
     */
    void updateLastActivityTime();

    /**
     * @brief 获取流的最后活动时间（毫秒）
     * @return 最后活动时间（毫秒时间戳）
     */
    int64_t getLastActivityTime() const;
    
    /**
     * @brief 检查流是否超时
     * @param timeoutMilliseconds 超时时间（毫秒）
     * @return 如果流超时返回true，否则返回false
     */
    bool isTimeout(size_t timeoutMilliseconds) const;

    /**
     * @brief 获取C2S方向的四元组
     * @return C2S方向的四元组
     */
    const FourTuple& getC2STuple() const { return c2sTuple; }
    
    /**
     * @brief 获取S2C方向的四元组
     * @return S2C方向的四元组
     */
    const FourTuple& getS2CTuple() const { return s2cTuple; }

private:
    FourTuple c2sTuple;                    // C2S方向的四元组
    FourTuple s2cTuple;                    // S2C方向的四元组
    std::vector<Message> c2sMessages;      // C2S方向解析出的消息
    std::vector<Message> s2cMessages;      // S2C方向解析出的消息
    std::vector<std::string> c2sState;     // C2S方向的状态
    std::vector<std::string> s2cState;     // S2C方向的状态
    CircularString c2sBuffer;              // C2S方向的数据缓冲区
    CircularString s2cBuffer;              // S2C方向的数据缓冲区
    int64_t lastActivityTime;              // 最后活动时间（毫秒时间戳）
};

/**
 * @brief 哈希流表类，管理所有网络流
 */
class HashFlowTable {
public:
    /**
     * @brief 构造函数
     */
    HashFlowTable();

    /**
     * @brief 析构函数
     */
    ~HashFlowTable();

    /**
     * @brief 根据四元组创建新流或获取已存在的流
     * @param fourTuple 四元组
     * @return 流对象指针
     */
    Flow* getOrCreateFlow(const FourTuple& fourTuple);

    /**
     * @brief 向流中添加数据包
     * @param packet 输入数据包
     * @return 是否成功处理
     */
    bool processPacket(const InputPacket& packet);

    /**
     * @brief 设置流超时时间
     * @param milliseconds 超时时间（毫秒）
     */
    void setFlowTimeout(size_t milliseconds);

    /**
     * @brief 检查并清理超时的流
     */
    void checkAndCleanupTimeoutFlows();
    
    /**
     * @brief 删除指定的流
     * @param flow 要删除的流指针
     */
    void deleteFlow(Flow* flow);

    /**
     * @brief 获取流总数
     * @return 流总数
     */
    size_t getTotalFlows() const;

    /**
     * @brief 输出所有流的处理结果
     */
    void outputResults() const;

    /**
     * @brief 获取所有流对象的引用
     * @return 所有不重复的流对象的向量
     */
    std::vector<Flow*> getAllFlows();

private:
    /**
     * @brief 计算四元组的哈希值
     * @param fourTuple 四元组
     * @return 哈希值
     */
    int hashFourTuple(const FourTuple& fourTuple) const;
    
    /**
     * @brief 将流添加到时间排序的链表中
     * @param flow 要添加的流
     */
    void addToTimeOrderedList(Flow* flow);

    /**
     * @brief 从时间排序的链表中移除流
     * @param flow 要移除的流
     */
    void removeFromTimeOrderedList(Flow* flow);

    /**
     * @brief 更新流在时间排序链表中的位置
     * @param flow 要更新的流
     */
    void updateFlowPosition(Flow* flow);

    std::unordered_map<int, Flow*> flowMap;               // 流映射表
    std::list<Flow*> timeOrderedFlows;                    // 按时间排序的流列表
    size_t flowTimeoutMilliseconds = 120000;              // 默认流超时时间120000毫秒（2分钟）
    
    // 时间桶数据结构（用于优化超时检查）
    using TimeBucket = std::unordered_set<Flow*>;         // 使用unordered_set
    std::map<int64_t, TimeBucket> timeBuckets;            // 按时间戳映射的流桶（毫秒）
    const int64_t BUCKET_INTERVAL = 1000;                 // 每个桶的时间间隔（毫秒）
    
    /**
     * @brief 添加流到时间桶中
     * @param flow 要添加的流
     * @param timestampMs 时间戳（毫秒）
     */
    void addToTimeBucket(Flow* flow, int64_t timestampMs);
    
    /**
     * @brief 从时间桶中移除流
     * @param flow 要移除的流
     * @param timestampMs 原先所在的时间戳（毫秒）
     */
    void removeFromTimeBucket(Flow* flow, int64_t timestampMs);
    
    /**
     * @brief 移动流到新的时间桶
     * @param flow 要移动的流
     * @param oldTimestampMs 原时间戳（毫秒）
     * @param newTimestampMs 新时间戳（毫秒）
     */
    void moveToNewTimeBucket(Flow* flow, int64_t oldTimestampMs, int64_t newTimestampMs);

};

} // namespace flow_table

#endif // FLOW_TABLE_FLOW_MANAGER_H
