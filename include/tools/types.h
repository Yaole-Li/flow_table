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

struct Header {
    std::string date;   //信件完成日期
    std::string from;   //发信人邮箱
    std::vector<std::string> sender;        //实际发信邮箱
    std::vector<std::string> reply_to;      //回复的目标邮箱
    std::vector<std::string> to;            //主要收件人邮箱
    std::vector<std::string> cc;            //次要收件人邮箱
    std::vector<std::string> bcc;           //匿名收件人邮箱
    std::vector<std::string> message_id;    //邮件ID
    std::vector<std::string> in_reply_to;   //回复邮件的邮件ID
    std::vector<std::string> references;    //邮件历史ID
    std::vector<std::string> subject;       //邮件标题
    std::vector<std::string> comments;      //邮件简介
    std::vector<std::string> keywords;      //关键词
    std::vector<std::string> resent_date;   //重投时间
    std::vector<std::string> resent_from;   //重投发信人邮箱
    std::vector<std::string> resent_sender; //重投实际发信邮箱
    std::vector<std::string> resent_to;     //重投主要收件人邮箱
    std::vector<std::string> resent_cc;     //重投次要收件人邮箱
    std::vector<std::string> resent_bcc;    //重投匿名收件人邮箱
    std::vector<std::string> resent_message_id;     //重投邮件ID
    std::vector<std::string> return_path;           //邮件退回地址
    std::vector<std::string> received;              //收发历史
    std::map<std::string, std::vector<std::string>> optional;    //其他没在RFC2822里定义的可选HEADER
};

struct Body {
    Header header;
    std::string text;
};

/**
 * @brief 邮件结构体，表示IMAP协议中的邮件结构
 * 
 * TODO: 此结构体将由Mr.zhao完善实现
 * 在完整实现中，应该包含邮件的所有信息，如信封信息、正文内容、附件等
 */
struct Email {
    //std::string body;
    Body body;
    std::string bodystructure;
    std::string envelope;
    std::string flags;
    std::string internaldate;
    size_t rfc822_size;
    size_t sequence_number;
    std::uint32_t uid;

    Email() : rfc822_size(0), sequence_number(0), uid(0) {}
};

enum class Fetch_name {
    BODYSTRUCTURE,
    ENVELOPE,
    FLAGS,
    INTERNALDATE,
    RFC822,
    RFC822_HEADER,
    RFC822_SIZE,
    RFC822_TEXT,
    UID
};

enum class Fetch_header {
    Date,
    From,
    Sender,
    Reply_To,
    To,
    Cc,
    Bcc,
    Message_ID,
    In_Reply_To,
    References,
    Subject,
    Comments,
    Keywords,
    Resent_Date,
    Resent_From,
    Resent_Sender,
    Resent_To,
    Resent_Cc,
    Resent_Bcc,
    Resent_Message_ID,
    Return_Path,
    Received,
};

const std::unordered_map<std::string, Fetch_name> fetch_name_index_map = {
    {"BODYSTRUCTURE", Fetch_name::BODYSTRUCTURE }, 
    {"ENVELOPE", Fetch_name::ENVELOPE }, 
    {"FLAGS", Fetch_name::FLAGS}, 
    {"INTERNALDATE", Fetch_name::INTERNALDATE},
    {"RFC822", Fetch_name::RFC822}, 
    {"RFC822.HEADER", Fetch_name::RFC822_HEADER}, 
    {"RFC822.SIZE", Fetch_name::RFC822_SIZE}, 
    {"RFC822.TEXT", Fetch_name::RFC822_TEXT}, 
    {"UID", Fetch_name::UID}
};

const std::unordered_map<std::string, Fetch_header> fetch_header_index_map = {
    {"Date", Fetch_header::Date},
    {"From", Fetch_header::From},
    {"Sender", Fetch_header::Sender},
    {"Reply-To", Fetch_header::Reply_To},
    {"To", Fetch_header::To},
    {"Cc", Fetch_header::Cc},
    {"Bcc", Fetch_header::Bcc},
    {"Message-ID", Fetch_header::Message_ID},
    {"In-Reply-To", Fetch_header::In_Reply_To},
    {"References", Fetch_header::References},
    {"Subject", Fetch_header::Subject},
    {"Comments", Fetch_header::Comments},
    {"Keywords", Fetch_header::Keywords},
    {"Resent-Date", Fetch_header::Resent_Date},
    {"Resent-From", Fetch_header::Resent_From},
    {"Resent-Sender", Fetch_header::Resent_Sender},
    {"Resent-To", Fetch_header::Resent_To},
    {"Resent-Cc", Fetch_header::Resent_Cc},
    {"Resent-Bcc", Fetch_header::Resent_Bcc},
    {"Resent-Message-ID", Fetch_header::Resent_Message_ID},
    {"Return-Path", Fetch_header::Return_Path},
    {"Received", Fetch_header::Received}
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