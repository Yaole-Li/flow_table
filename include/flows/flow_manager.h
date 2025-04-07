#ifndef FLOW_TABLE_FLOW_MANAGER_H
#define FLOW_TABLE_FLOW_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
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
     * @brief 构造函数
     * @param c2sTuple C2S方向的四元组
     * @param s2cTuple S2C方向的四元组
     */
    Flow(const FourTuple& c2sTuple, const FourTuple& s2cTuple);

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
     * @brief 标记流可以删除
     */
    void markForDeletion();

    /**
     * @brief 检查流是否标记为删除
     * @return 流是否可以删除
     */
    bool isMarkedForDeletion() const;

private:
    FourTuple c2sTuple;                    // C2S方向的四元组
    FourTuple s2cTuple;                    // S2C方向的四元组
    std::vector<Message> c2sMessages;      // C2S方向解析出的消息
    std::vector<Message> s2cMessages;      // S2C方向解析出的消息
    std::vector<std::string> c2sState;     // C2S方向的状态
    std::vector<std::string> s2cState;     // S2C方向的状态
    CircularString c2sBuffer;              // C2S方向的数据缓冲区
    CircularString s2cBuffer;              // S2C方向的数据缓冲区
    bool markedForDeletion;                // 是否标记为删除
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
     * @brief 清理标记为删除的流
     */
    void cleanupMarkedFlows();

    /**
     * @brief 获取流总数
     * @return 流总数
     */
    size_t getTotalFlows() const;

    /**
     * @brief 输出所有流的处理结果
     */
    void outputResults() const;

private:
    /**
     * @brief 计算四元组的哈希值
     * @param fourTuple 四元组
     * @return 哈希值
     */
    int hashFourTuple(const FourTuple& fourTuple) const;

    std::unordered_map<int, Flow*> flowMap;  // 流映射表
};

} // namespace flow_table

#endif // FLOW_TABLE_FLOW_MANAGER_H
