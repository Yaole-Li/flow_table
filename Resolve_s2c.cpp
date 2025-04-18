
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <cctype>
#include <cstdint>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "CircularString.h"
#if defined(_WIN32) || defined(_MSC_VER) || defined(WIN64) 
#include <Windows.h>
#elif defined(__linux__) || defined(__GNUC__)
#include <iconv.h>
#endif

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

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

struct Message {
    std::string tag;                  // 消息标签
    std::string command;              // 命令或响应类型
    std::vector<std::string> args;    // 参数列表
    std::vector<struct Email> fetch;  // 提取的邮件列表
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



std::vector<Message> c2sMessages;      // C2S方向解析出的消息
std::vector<Message> s2cMessages;      // S2C方向解析出的消息
int Reslove_s2c(CircularString& cstring);

#if defined(__linux__) || defined(__GNUC__)
int EncodingConvert(const char* charsetSrc, const char* charsetDest, char* inbuf,
    size_t inSz, char* outbuf, size_t outSz)
{

    iconv_t cd;
    char** pin = &inbuf;
    char** pout = &outbuf;
    cd = iconv_open(charsetDest, charsetSrc);
    if (0 == cd)
    {

        std::cerr << charsetSrc << " to " << charsetDest
            << " conversion not available" << std::endl;
        return -1;
    }

    if (-1 == static_cast<int>(iconv(cd, pin, &inSz, pout, &outSz)))
    {

        std::cerr << "conversion failure" << std::endl;
        return -1;
    }

    iconv_close(cd);
    **pout = '\0';
    return 0;
}
#endif

std::string GbkToUtf8(const std::string& str)
{

#if defined(_WIN32) || defined(_MSC_VER) || defined(WIN64)
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1ull];
    memset(wstr, 0, len + 1ull);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* cstr = new char[len + 1ull];
    memset(cstr, 0, len + 1ull);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, cstr, len, NULL, NULL);
    std::string res(cstr);

    if (wstr) delete[] wstr;
    if (cstr) delete[] cstr;

    return res;
#elif defined(__linux__) || defined(__GNUC__)
    size_t len = str.size() * 2 + 1;
    char* temp = new char[len];
    if (EncodingConvert("gb2312", "utf-8", const_cast<char*>(str.c_str()), str.size(), temp, len)
        > = 0)
    {

        std::string res;
        res.append(temp);
        delete[] temp;
        return res;
    }
    else
    {

        delete[]temp;
        return str;
    }
#else
    std::cerr << "Unhandled operating system." << std::endl;
    return str;
#endif
}

std::string Utf8ToGbk(const std::string& str)
{

#if defined(_WIN32) || defined(_MSC_VER) || defined(WIN64) 
    // calculate length
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* wsGbk = new wchar_t[len + 1ull];
    // set to '\0'
    memset(wsGbk, 0, len + 1ull);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wsGbk, len);
    len = WideCharToMultiByte(CP_ACP, 0, wsGbk, -1, NULL, 0, NULL, NULL);
    char* csGbk = new char[len + 1ull];
    memset(csGbk, 0, len + 1ull);
    WideCharToMultiByte(CP_ACP, 0, wsGbk, -1, csGbk, len, NULL, NULL);
    std::string res(csGbk);

    if (wsGbk)
    {

        delete[] wsGbk;
    }

    if (csGbk)
    {

        delete[] csGbk;
    }

    return res;
#elif defined(__linux__) || defined(__GNUC__)
    size_t len = str.size() * 2 + 1;
    char* temp = new char[len];
    if (EncodingConvert("utf-8", "gb2312", const_cast<char*>(str.c_str()),
        str.size(), temp, len) >= 0)
    {

        std::string res;
        res.append(temp);
        delete[] temp;
        return res;
    }
    else
    {

        delete[] temp;
        return str;
    }

#else
    std::cerr << "Unhandled operating system." << std::endl;
    return str;
#endif
}




std::string Base64_decode(const std::string& input) {
    BIO* bio, * b64;
    char* buffer = new char[input.length()];
    memset(buffer, 0, input.length());

    // 创建一个 Base64 解码的 BIO 链
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input.c_str(), input.length());
    bio = BIO_push(b64, bio);

    // 禁止读取换行符
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    // 读取解码后的数据
    int length = BIO_read(bio, buffer, input.length());

    // 转换为字符串
    std::string result(buffer, length);

    // 释放资源
    delete[] buffer;
    BIO_free_all(bio);

    return result;
}

//#include <Windows.h>
//int main() {
//
//
//    SetConsoleOutputCP(CP_UTF8);
//    std::string encoded_string = "DQrmiJHnu4Plip/lj5Hoh6rnnJ/lv4M="; // "Hello World!" in Base64
//    std::string decoded_string = Base64_decode(encoded_string);
//
//    std::cout << "Encoded: " << encoded_string << std::endl;
//    std::cout << "Decoded: " << GbkToUtf8(Utf8ToGbk(decoded_string)) << std::endl;
//
//    return 0;
//}
int Resolve_imap_body(std::string& str, Email &current_email, bool has_header, bool has_text) {
    size_t left = 0;
    size_t middle = 0;
    size_t right = -2;
    std::string current_header;
    std::string value_of_header;
    if (has_header == true) {
        std::string current_header;
        std::string value_of_header;
        right = str.find("\r\n");
        while (left != right && right != std::string::npos) {
            try {
                middle = str.find(':', left);
                if (middle > right) {
                    throw std::runtime_error("HEADER中没有找到冒号");
                }
                else {
                    current_header = str.substr(left, middle - left);
                    while (str.at(right + 2) == ' ' || str.at(right + 2) == 9) {
                        left = right + 2;
                        right = str.find("\r\n", left);
                    }
                    value_of_header = str.substr(middle + 1, right - middle - 1);
                    auto it2 = fetch_header_index_map.find(current_header);
                    if (it2 != fetch_header_index_map.end()) {
                        switch (it2->second) {
                        case Fetch_header::Date:
                            current_email.body.header.date = value_of_header;
                            break;
                        case Fetch_header::From:
                            current_email.body.header.from = value_of_header;
                            break;
                        case Fetch_header::Sender:
                            current_email.body.header.sender.push_back(value_of_header);
                            break;
                        case Fetch_header::Reply_To:
                            current_email.body.header.reply_to.push_back(value_of_header);
                            break;
                        case Fetch_header::To:
                            current_email.body.header.to.push_back(value_of_header);
                            break;
                        case Fetch_header::Cc:
                            current_email.body.header.cc.push_back(value_of_header);
                            break;
                        case Fetch_header::Bcc:
                            current_email.body.header.bcc.push_back(value_of_header);
                            break;
                        case Fetch_header::Message_ID:
                            current_email.body.header.message_id.push_back(value_of_header);
                            break;
                        case Fetch_header::In_Reply_To:
                            current_email.body.header.in_reply_to.push_back(value_of_header);
                            break;
                        case Fetch_header::References:
                            current_email.body.header.references.push_back(value_of_header);
                            break;
                        case Fetch_header::Subject:
                            current_email.body.header.subject.push_back(value_of_header);
                            break;
                        case Fetch_header::Comments:
                            current_email.body.header.comments.push_back(value_of_header);
                            break;
                        case Fetch_header::Keywords:
                            current_email.body.header.keywords.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_Date:
                            current_email.body.header.resent_date.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_From:
                            current_email.body.header.resent_from.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_Sender:
                            current_email.body.header.resent_sender.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_To:
                            current_email.body.header.resent_to.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_Cc:
                            current_email.body.header.resent_cc.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_Bcc:
                            current_email.body.header.resent_bcc.push_back(value_of_header);
                            break;
                        case Fetch_header::Resent_Message_ID:
                            current_email.body.header.resent_message_id.push_back(value_of_header);
                            break;
                        case Fetch_header::Return_Path:
                            current_email.body.header.return_path.push_back(value_of_header);
                            break;
                        case Fetch_header::Received:
                            current_email.body.header.received.push_back(value_of_header);
                            break;
                        }
                    }
                    //其他HEADER
                    else {
                        auto it3 = current_email.body.header.optional.find(current_header);
                        if (it3 != current_email.body.header.optional.end()) {
                            it3->second.push_back(value_of_header);
                        }
                        else {
                            current_email.body.header.optional.emplace(current_header, std::vector < std::string >({ value_of_header }));

                        }
                    }
                }

            }
            catch (std::out_of_range e) {
                //HEADER格式不对，解析不下去了
                left = right = str.size();
                //current_email.body.text = "邮件解析失败";
                std::cerr << "邮件头部解析失败";
                return 1;
            }
            left = right + 2;
            right = str.find("\r\n", left);
        }
        if (left != right) {
            left = right = str.size();
            //current_email.body.text = "邮件解析失败";
            std::cerr << "邮件头部解析失败";
            return 1;
        }
    }
    //剩下的是正文
    if (has_text == true) {
        if (right + 2 < str.size()) {
            current_email.body.text = str.substr(right + 2);
        }
    }
    return 0;
}

int Reslove_s2c(CircularString& cstring) {
    Email current_email;
    size_t email_count = 0;
    Message current_message;    //存储当前请求信息的结构体，如果解析正常结束则将其添加到c2sMessages尾端
    std::string temp;           //用于暂存解析参数时遇到的字符串
    size_t index = 0;              //下一个要从line中读取的字符的位置
    int left_bracket_count = 0;         //用于解析参数时的括号匹配
    char curren_char = 0;    //当前正在被解析的字符
    size_t current_number = 0;
    size_t(end_line_index);


    while (1) {
        index = 0;  //索引重新指向缓冲区起点
        current_email = Email();    //清空Email结构体中的数据
        try {
            //判断响应类型
            curren_char = cstring.at(index++);
            //+ 代表 Command Contuination Request，目前直接无视
            if (curren_char == '+') {
                throw std::runtime_error("目前暂时不处理+开头的响应");
                continue;
            }
            // * 代表单方向响应，目前只关注FETCH响应
            else if (curren_char == '*') {
                curren_char = cstring.at(index++);
                //删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (curren_char != ' ' && curren_char != 9) {
                    throw std::runtime_error("没有找到空白符");
                    continue;
                }
                do{
                    curren_char = cstring.at(index++);
                } while (curren_char == ' ' || curren_char == 9);
                //Fetch响应包的第二项是一个数字，代表邮件的序号
                if (!std::isdigit(curren_char)) {
                    throw std::runtime_error("这不是一个Fetch响应包");
                    continue;
                }
                current_number = 0;
                do {
                    current_number = current_number * 10 + curren_char - '0';
                    curren_char = cstring.at(index++);
                } while (std::isdigit(curren_char));
                current_email.sequence_number = current_number;
                //删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (curren_char != ' ' && curren_char != 9) {
                    throw std::runtime_error("这不是一个Fetch响应包！");
                    continue;
                }
                do {
                    curren_char = cstring.at(index++);
                } while (curren_char == ' ' || curren_char == 9);
                //判断这个字段是不是FETCH
                temp.clear();
                if (curren_char >= 33 && curren_char <= 126) {
                    temp.push_back(std::toupper(curren_char));
                    curren_char = cstring.at(index++);
                    while (curren_char >= 33 && curren_char <= 126) {
                        temp.push_back(std::toupper(curren_char));
                        curren_char = cstring.at(index++);
                    }
                }
                else {
                    throw std::runtime_error("这不是一个Fetch响应包！！");
                    continue;
                }
                if (temp != "FETCH") {
                    throw std::runtime_error("这不是一个Fetch响应包！！！");
                    continue;
                }
                //删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (curren_char != ' ' && curren_char != 9) {
                    throw std::runtime_error("没有找到空白符");
                    continue;
                }
                do {
                    curren_char = cstring.at(index++);
                } while (curren_char == ' ' || curren_char == 9);
                //
                // 读取键值对
                //
                if (curren_char != '(') {
                    throw std::runtime_error("Fetch后没找到左括号");
                }
                curren_char = std::toupper(cstring.at(index++));
                //循环读取键值对，直到遇到右括号
                while (curren_char != ')') {
                    temp = "";  //准备读取键
                    if (curren_char >= 33 && curren_char <= 126) {
                        do {
                            if (curren_char == '(') {
                                temp.push_back(curren_char);
                                curren_char = std::toupper(cstring.at(index++));
                                left_bracket_count = 1;
                                while (left_bracket_count > 0) {
                                    if (curren_char >= 32 && curren_char <= 126) {
                                        if (curren_char == '(') {
                                            left_bracket_count++;
                                        }
                                        else if (curren_char == ')') {
                                            left_bracket_count--;
                                        }
                                        temp.push_back(curren_char);
                                        curren_char = std::toupper(cstring.at(index++));
                                    }
                                    else {
                                        throw std::runtime_error("键中发现不可打印字符");
                                    }
                                }
                            }
                            else if(curren_char == '[') {
                                temp.push_back(curren_char);
                                curren_char = std::toupper(cstring.at(index++));
                                left_bracket_count = 1;
                                while (left_bracket_count > 0) {
                                    if (curren_char >= 32 && curren_char <= 126) {
                                        if (curren_char == '[') {
                                            left_bracket_count++;
                                        }
                                        else if (curren_char == ']') {
                                            left_bracket_count--;
                                        }
                                        temp.push_back(curren_char);
                                        curren_char = std::toupper(cstring.at(index++));
                                    }
                                    else {
                                        throw std::runtime_error("键中发现不可打印字符");
                                    }
                                }
                            }
                            else {
                                temp.push_back(curren_char);
                                curren_char = std::toupper(cstring.at(index++));
                            }
                        } while (curren_char >= 33 && curren_char <= 126);
                    }
                    else {
                        throw std::runtime_error("Fetch没找到键的字符");
                    }
                    //删除空白符（空格和水平制表符）,且至少得有一个空白符
                    if (curren_char != ' ' && curren_char != 9) {
                        throw std::runtime_error("没有找到空白符");
                        continue;
                    }
                    do {
                        curren_char = cstring.at(index++);
                    } while (curren_char == ' ' || curren_char == 9);
                    //根据不同的键，采用不同的方式读取值
                    auto it = fetch_name_index_map.find(temp);
                    if (it != fetch_name_index_map.end()) {
                        
                        switch (it->second) {
                        case Fetch_name::BODYSTRUCTURE:
                            //假设bodystructure的值在一对括号内
                            if (curren_char != '(') {
                                throw std::runtime_error("BODYSTRUCTURE值的首个字符不是左括号");
                            }
                            current_email.bodystructure.push_back(curren_char);
                            curren_char = cstring.at(index++);
                            left_bracket_count = 1;
                            while (left_bracket_count > 0) {
                                if (curren_char >= 32 && curren_char <= 126) {
                                    if (curren_char == '(') {
                                        left_bracket_count++;
                                    }
                                    else if (curren_char == ')') {
                                        left_bracket_count--;
                                    }
                                    current_email.bodystructure.push_back(curren_char);
                                    curren_char = cstring.at(index++);
                                }
                                else {
                                    throw std::runtime_error("BODYSTRUCTURE值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::ENVELOPE:
                            //假设envelope的值在一对括号内
                            if (curren_char != '(') {
                                throw std::runtime_error("ENVELOPE值的首个字符不是左括号");
                            }
                            current_email.envelope.push_back(curren_char);
                            curren_char = cstring.at(index++);
                            left_bracket_count = 1;
                            while (left_bracket_count > 0) {
                                if (curren_char >= 32 && curren_char <= 126) {
                                    if (curren_char == '(') {
                                        left_bracket_count++;
                                    }
                                    else if (curren_char == ')') {
                                        left_bracket_count--;
                                    }
                                    current_email.envelope.push_back(curren_char);
                                    curren_char = cstring.at(index++);
                                }
                                else {
                                    throw std::runtime_error("ENVELOPE值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::FLAGS:
                            //假设flags的值在一对括号内
                            if (curren_char != '(') {
                                throw std::runtime_error("FLAGS值的首个字符不是左括号");
                            }
                            current_email.flags.push_back(curren_char);
                            curren_char = cstring.at(index++);
                            left_bracket_count = 1;
                            while (left_bracket_count > 0) {
                                if (curren_char >= 32 && curren_char <= 126) {
                                    if (curren_char == '(') {
                                        left_bracket_count++;
                                    }
                                    else if (curren_char == ')') {
                                        left_bracket_count--;
                                    }
                                    current_email.flags.push_back(curren_char);
                                    curren_char = cstring.at(index++);
                                }
                                else {
                                    throw std::runtime_error("FLAGS值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::INTERNALDATE:
                            //假设INTERNALDATE的值在一对双引号内
                            if (curren_char != '\"') {
                                throw std::runtime_error("FLAGS值的首个字符不是双引号");
                            }
                            curren_char = cstring.at(index++);
                            while (curren_char != '\"') {
                                if (curren_char >= 32 && curren_char <= 126) {
                                    current_email.internaldate.push_back(curren_char);
                                    curren_char = cstring.at(index++);
                                }
                                else {
                                    throw std::runtime_error("FLAGS值内发现不可打印字符");
                                }
                            }
                            curren_char = cstring.at(index++);
                            break;
                        case  Fetch_name::RFC822:  //等同于BODY[]
                            //假设RFC822的值是literal字符串的格式
                            if (curren_char != '{') {
                                throw std::runtime_error("RFC822的值不是以'{'开头");
                            }
                            curren_char = cstring.at(index++);
                            //计算RFC822的字节数
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("RFC822的值的'{'后不是数字");
                                continue;
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            if (curren_char != '}') {
                                throw std::runtime_error("RFC822的值的\"{数字\"后不是'}'");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\r') {
                                throw std::runtime_error("RFC822的值的\"{数字}\"后不是换行");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\n') {
                                throw std::runtime_error("RFC822的值的\"{数字}\"换行后不是回车");
                            }
                            cstring.at(index + current_number - 1); //试一下读取字符串末尾会不会越界，以免白忙活
                            curren_char = cstring.at(index++);
                            //先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < current_number;i++) {
                                temp.push_back(curren_char);
                                curren_char = cstring.at(index++);
                            }
                            Resolve_imap_body(temp, current_email, true, true);
                            break;
                        case  Fetch_name::RFC822_HEADER:  //等同于BODY.PEEK[HEADER]
                            //假设RFC822.HEADER的值是literal字符串的格式
                            if (curren_char != '{') {
                                throw std::runtime_error("RFC822.HEADER的值不是以'{'开头");
                            }
                            curren_char = cstring.at(index++);
                            //计算RFC822.HEADER的字节数
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("RFC822.HEADER的值的'{'后不是数字");
                                continue;
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            if (curren_char != '}') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字\"后不是'}'");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\r') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字}\"后不是换行");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\n') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字}\"换行后不是回车");
                            }
                            cstring.at(index + current_number - 1); //试一下读取字符串末尾会不会越界，以免白忙活
                            curren_char = cstring.at(index++);
                            //先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < current_number;i++) {
                                temp.push_back(curren_char);
                                curren_char = cstring.at(index++);
                            }
                            //先处理头部信息
                            Resolve_imap_body(temp, current_email, true, false);
                            break;
                        case  Fetch_name::RFC822_SIZE:
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("RFC822_SIZE的值不是数字");
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            current_email.rfc822_size = current_number;
                            break;
                        case  Fetch_name::RFC822_TEXT:  //等同于BODY[TEXT]
                            //假设RFC822.TEXT的值是literal字符串的格式
                            if (curren_char != '{') {
                                throw std::runtime_error("RFC822.TEXT的值不是以'{'开头");
                            }
                            curren_char = cstring.at(index++);
                            //计算RFC822.TEXT的字节数
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("RFC822.TEXT的值的'{'后不是数字");
                                continue;
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            if (curren_char != '}') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字\"后不是'}'");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\r') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字}\"后不是换行");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\n') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字}\"换行后不是回车");
                            }
                            cstring.at(index + current_number - 1); //试一下读取字符串末尾会不会越界，以免白忙活
                            curren_char = cstring.at(index++);
                            current_email.body.text = "";
                            // 字符串里的内容都是正文
                            for (int i = 0;i < current_number;i++) {
                                current_email.body.text.push_back(curren_char);
                                curren_char = cstring.at(index++);
                            }
                            break;
                        case  Fetch_name::UID:
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("UID的值不是数字");
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            current_email.uid = (std::uint32_t)(current_number);
                            break;
                        }// End switch
                        //找到下一个非空白符（空格和水平制表符）
                        while (curren_char == ' ' || curren_char == 9) {
                            curren_char = cstring.at(index++);
                        }
                    }//End if
                    else {
                        if (temp.compare(0,5,"BODY[") != 0) {
                            throw std::runtime_error("Fetch响应中含有未知的键");
                        }
                        //到这里就已经确定键是BODY[...]或者BODY[...]<>的格式了
                        else {
                            bool has_herader = false;
                            bool has_text = false;
                            //假设Header的名称中都不包含括号，也就是BODY[...]中只可能有HEADER.FILEDS (...) 这一对括号
                            size_t left_bracket_index = temp.find('(',5);
                            size_t right_bracket_index = temp.find(')',5);
                            //在BODY[...]的中括号内，圆括号外找有没有HEADER字段和TEXT字段
                            if (left_bracket_index != std::string::npos && right_bracket_index != std::string::npos) {
                                has_herader = temp.find("HEADER") > left_bracket_index ? has_herader : true;
                                if (has_herader == false) {
                                    has_herader = temp.find("HEADER",right_bracket_index + 1) == std::string::npos ? has_herader : true;
                                }
                                has_text = temp.find("TEXT") > left_bracket_index ? has_text : true;
                                if (has_text == false) {
                                    has_text = temp.find("TEXT", right_bracket_index + 1) == std::string::npos ? has_text : true;
                                }
                            }
                            else {
                                has_herader = temp.find("HEADER") == std::string::npos ? has_herader : true;
                                has_text = temp.find("TEXT") == std::string::npos ? has_text : true;
                            }
                            if (has_herader == false && has_text == false) {
                                has_herader = true;
                                has_text = true;
                            }
                            //假设BODY的值是literal字符串的格式
                            if (curren_char != '{') {
                                throw std::runtime_error("BODY的值不是以'{'开头");
                            }
                            curren_char = cstring.at(index++);
                            //计算BODY的字节数
                            if (!std::isdigit(curren_char)) {
                                throw std::runtime_error("BODY的值的'{'后不是数字");
                                continue;
                            }
                            current_number = 0;
                            do {
                                current_number = current_number * 10 + curren_char - '0';
                                curren_char = cstring.at(index++);
                            } while (std::isdigit(curren_char));
                            if (curren_char != '}') {
                                throw std::runtime_error("BODY的值的\"{数字\"后不是'}'");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\r') {
                                throw std::runtime_error("BODY的值的\"{数字}\"后不是换行");
                            }
                            curren_char = cstring.at(index++);
                            if (curren_char != '\n') {
                                throw std::runtime_error("BODY的值的\"{数字}\"换行后不是回车");
                            }
                            cstring.at(index + current_number - 1); //试一下读取字符串末尾会不会越界，以免白忙活
                            curren_char = cstring.at(index++);
                            //先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < current_number;i++) {
                                temp.push_back(curren_char);
                                curren_char = cstring.at(index++);
                            }
                            Resolve_imap_body(temp, current_email, has_herader, has_text);
                            //找到下一个非空白符（空格和水平制表符）
                            while (curren_char == ' ' || curren_char == 9) {
                                curren_char = cstring.at(index++);
                            }
                        }
                    }
                }
                //找回车换行
                end_line_index = cstring.find(index, cstring.size(), '\r');
                if (end_line_index == (size_t)(-1)) {
                    throw std::out_of_range("Fetch的右括号后没找到换行符");
                }
                if (cstring.at(end_line_index + 1) != '\n') {
                    throw std::runtime_error("回车换行不匹配");
                }
                
                //试着对邮件内容进行解码
                if (current_email.body.text != "") {
                    auto it4 = current_email.body.header.optional.find("Content-Type");
                    if (it4 != current_email.body.header.optional.end()) {
                        if (it4->second[0].find("text/plain") != std::string::npos) {
                            current_email.body.text = Base64_decode(current_email.body.text);
                            if (it4->second[0].find("charset = \"gb18030\"") != std::string::npos) {
                                current_email.body.text = GbkToUtf8(current_email.body.text);
                            }
                        }
                        
                    }
                }

                //做一些收尾操作，然后continue读取下一封邮件
                current_message.fetch.push_back(current_email);
                email_count++;
                cstring.erase_up_to(end_line_index + 1);
                std::cout << "成功完成一封邮件的解析" << std::endl;
                continue;
            }
            //正常的响应类型，结构为 <Tag> <Status> <text>
            else if (curren_char >= 33 && curren_char <= 126) {
                // 解析标签的第一个字节
                current_message.tag.push_back(curren_char);
                curren_char = cstring.at(index++);
                //解析标签的剩余字节
                while (curren_char >= 33 && curren_char <= 126) {
                    current_message.tag.push_back(curren_char);
                    curren_char = cstring.at(index++);
                }
                //删除标签和状态回复之间的空白符,且至少得有一个空白符
                if (curren_char != ' ' && curren_char != 9) {
                    throw std::runtime_error("没有找到标签和状态回复中间的空白符");
                    continue;
                }
                do {
                    curren_char = cstring.at(index++);
                } while (curren_char == ' ' || curren_char == 9);
                // 解析状态回复（"OK" "NO" "BAD"）的第一个字节
                curren_char = std::toupper(curren_char);
                if (curren_char == 'O') {
                    current_message.command.push_back(curren_char);
                    curren_char = std::toupper(cstring.at(index++));
                    if (curren_char == 'K') {
                        current_message.command.push_back(curren_char);
                        curren_char = cstring.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else if (curren_char == 'N' && curren_char != 'B') {
                    current_message.command.push_back(curren_char);
                    curren_char = std::toupper(cstring.at(index++));
                    if (curren_char == 'O') {
                        current_message.command.push_back(curren_char);
                        curren_char = cstring.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else if (curren_char == 'B') {
                    current_message.command.push_back(curren_char);
                    curren_char = std::toupper(cstring.at(index++));
                    if (curren_char == 'A') {
                        current_message.command.push_back(curren_char);
                        curren_char = std::toupper(cstring.at(index++));
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                    if (curren_char == 'D') {
                        current_message.command.push_back(curren_char);
                        curren_char = cstring.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else {
                    throw std::runtime_error("状态回复不合法");
                }
                if (curren_char >= 33 && curren_char <= 126) {
                    throw std::runtime_error("状态回复不合法");
                }
                //删除状态回复和文本之间的空白符（空格和水平制表符）
                while (curren_char == ' ' || curren_char == 9) {
                    curren_char = cstring.at(index++);
                }
                //找到行末尾的换行符
                end_line_index = cstring.find(index - 1, cstring.size(), '\r');
                if (end_line_index == (size_t)(-1)) {
                    throw std::out_of_range("缓冲区内未找到换行符");
                }
                if (cstring.at(end_line_index + 1) != '\n') {
                    throw std::runtime_error("回车换行不匹配");
                }
                //把到换行符为止的字符一股脑存进args[0]中
                temp = "";
                while (index <= end_line_index) {
                    temp.push_back(curren_char);
                    curren_char = cstring.at(index++);
                }
                current_message.args.push_back(temp);
                s2cMessages.push_back(current_message);     //保存message
                //清理缓冲区
                cstring.erase_up_to(end_line_index + 1);
                break;
            }
            //以上三种情况都不满足说明这根本不是一个合法的响应
            else {
                throw std::out_of_range("非法响应格式");
                continue;
            }
        }
        catch (const std::out_of_range e) {
            if (email_count > 0) {
                s2cMessages.push_back(current_message);
            }
            return -1;
        }
        catch (const std::runtime_error& e) {
            //找到行末尾的换行符
            end_line_index = cstring.find((index - 1 > 0 ? index - 1 : 0), cstring.size(), '\r');
            if (end_line_index == (size_t)(-1)) {
                // 情况一：响应语句不合法，试图删除整行语句时却发现buffer里压根找不到换行符
                return -1;  //表达buffer里的数据不够用
            }
            try {
                if (cstring.at(end_line_index + 1) != '\n') {
                    //情况二：响应语句不合法，能在buffer里找到换行符，但下一个字节不是回车
                    std::cerr << e.what() << ':';
                    for (int i = 0;i < end_line_index;i++) {
                        if (cstring.at(i) >= 32 && cstring.at(i) <= 126) {
                            std::cerr << cstring.at(i);
                        }
                        else {
                            std::cerr << "\\0x" << std::setw(2) << std::setfill('0') << std::hex << int(cstring.at(i));
                        }
                    }
                    std::cerr << std::endl << std::dec;
                    cstring.erase_up_to(end_line_index);     // 清理缓冲区
                }
                else {
                    //情况三：响应语句不合法，能在buffer里找到换行符，且下一个字节是回车
                    std::cerr << e.what() << ':';
                    for (int i = 0;i < end_line_index;i++) {
                        if (cstring.at(i) >= 32 && cstring.at(i) <= 126) {
                            std::cerr << cstring.at(i);
                        }
                        else {
                            std::cerr << "\\0x" << std::setw(2) << std::setfill('0') << std::hex << int(cstring.at(i));
                        }
                    }
                    std::cerr << std::endl << std::dec;
                    cstring.erase_up_to(end_line_index + 1);     // 清理缓冲区
                }
            }
            catch (std::out_of_range e) {
                //情况四：响应语句不合法，能在buffer里找到换行符，但由于换行符在缓冲区结尾所以不知道下一个字符是不是回车
                return -1; //表达buffer里的数据不够用
            }
            continue;
        }
    }
    return 0;
}