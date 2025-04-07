#ifndef FLOW_TABLE_TYPES_H
#define FLOW_TABLE_TYPES_H

#include <string>
#include <vector>

/**
 * @brief 消息结构体，表示IMAP命令或响应
 */
struct Message {
    std::string tag;                  // 消息标签
    std::string command;              // 命令或响应类型
    std::vector<std::string> args;    // 参数列表
    std::vector<struct Email> fetch;  // 提取的邮件列表
};

/**
 * @brief 邮件结构体，表示IMAP协议中的邮件结构
 * 
 * TODO: 此结构体将由Mr.zhao完善实现
 * 在完整实现中，应该包含邮件的所有信息，如信封信息、正文内容、附件等
 */
struct Email {
    // 待实现 - Mr.zhao
};

/**
 * @brief 四元组结构体，表示网络流的唯一标识
 */
struct FourTuple {
    // 源端
    unsigned char srcIPvN;            // 源IP版本 4/6
    union {
        unsigned int srcIPv4;         // 源IPv4地址
        unsigned char srcIPv6[16];    // 源IPv6地址
    };
    int sourcePort;                   // 源端口
    
    // 目标端
    unsigned char dstIPvN;            // 目标IP版本 4/6
    union {
        unsigned int dstIPv4;         // 目标IPv4地址
        unsigned char dstIPv6[16];    // 目标IPv6地址
    };
    int destPort;                     // 目标端口

    /**
     * @brief 判断两个四元组是否相等
     */
    bool operator==(const FourTuple& other) const {
        // 如果IP版本不同，直接返回不相等
        if (srcIPvN != other.srcIPvN || dstIPvN != other.dstIPvN) {
            return false;
        }
        
        // 检查端口
        if (sourcePort != other.sourcePort || destPort != other.destPort) {
            return false;
        }
        
        // 检查IP地址
        if (srcIPvN == 4) {
            // IPv4比较
            if (srcIPv4 != other.srcIPv4 || dstIPv4 != other.dstIPv4) {
                return false;
            }
        } else if (srcIPvN == 6) {
            // IPv6比较
            for (int i = 0; i < 16; i++) {
                if (srcIPv6[i] != other.srcIPv6[i] || dstIPv6[i] != other.dstIPv6[i]) {
                    return false;
                }
            }
        }
        
        return true;
    }
};

#endif // FLOW_TABLE_TYPES_H