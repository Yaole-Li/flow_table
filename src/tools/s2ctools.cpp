#include <tools/s2ctools.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#if defined(_WIN32) || defined(_MSC_VER) || defined(WIN64) 
#include <Windows.h>
#elif defined(__linux__) || defined(__GNUC__)
#include <iconv.h>
#endif

namespace flow_table {

#if defined(__linux__) || defined(__GNUC__)
int Encoding_convert(const char* charsetSrc, const char* charsetDest, char* inbuf,
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

std::string Gbk_to_utf8(const std::string& str)
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
    size_t inSz = str.size();
    size_t outSz = inSz * 3;
    char* inbuf = new char[inSz + 1ull];
    char* outbuf = new char[outSz + 1ull];
    memset(inbuf, 0, inSz + 1ull);
    memset(outbuf, 0, outSz + 1ull);
    memcpy(inbuf, str.c_str(), inSz);
    Encoding_convert("gbk", "utf-8", inbuf, inSz, outbuf, outSz);
    std::string res(outbuf);
    delete[] inbuf;
    delete[] outbuf;
    return res;
#else
    return str;
#endif
}

std::string Utf8_to_gbk(const std::string& str)
{
#if defined(_WIN32) || defined(_MSC_VER) || defined(WIN64)
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1ull];
    memset(wstr, 0, len + 1ull);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, len);
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* cstr = new char[len + 1ull];
    memset(cstr, 0, len + 1ull);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, cstr, len, NULL, NULL);
    std::string res(cstr);

    if (wstr) delete[] wstr;
    if (cstr) delete[] cstr;
    return res;
#elif defined(__linux__) || defined(__GNUC__)
    size_t inSz = str.size();
    size_t outSz = inSz * 3;
    char* inbuf = new char[inSz + 1ull];
    char* outbuf = new char[outSz + 1ull];
    memset(inbuf, 0, inSz + 1ull);
    memset(outbuf, 0, outSz + 1ull);
    memcpy(inbuf, str.c_str(), inSz);
    Encoding_convert("utf-8", "gbk", inbuf, inSz, outbuf, outSz);
    std::string res(outbuf);
    delete[] inbuf;
    delete[] outbuf;
    return res;
#else
    return str;
#endif
}

std::string Base64_decode(const std::string& input)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(input.c_str(), input.length());
    bmem = BIO_push(b64, bmem);
    
    char* buffer = new char[input.length()];
    memset(buffer, 0, input.length());
    int decoded_length = BIO_read(bmem, buffer, input.length());
    
    std::string result(buffer, decoded_length);
    
    BIO_free_all(bmem);
    delete[] buffer;
    
    return result;
}

int Resolve_imap_body(std::string& str, Email& current_email, bool has_header, bool has_text)
{
    size_t left = 0;
    size_t right = 0;
    
    // 处理头部信息
    if (has_header == true) {
        while (right < str.size()) {
            // 找到下一个冒号的位置
            right = str.find(':', left);
            if (right == std::string::npos) {
                break;
            }
            
            // 提取键名
            std::string key = str.substr(left, right - left);
            
            // 去除键名前后的空白字符
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            
            // 找到值的结束位置（下一个换行符）
            left = right + 1;
            right = str.find("\r\n", left);
            if (right == std::string::npos) {
                break;
            }
            
            // 提取值
            std::string value = str.substr(left, right - left);
            
            // 去除值前后的空白字符
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 处理多行值（如果下一行以空格或制表符开头）
            while (right + 2 < str.size() && (str[right + 2] == ' ' || str[right + 2] == '\t')) {
                left = right + 2;
                right = str.find("\r\n", left);
                if (right == std::string::npos) {
                    break;
                }
                
                std::string continuation = str.substr(left, right - left);
                continuation.erase(0, continuation.find_first_not_of(" \t"));
                value += " " + continuation;
            }
            
            // 根据键名存储值
            auto it = fetch_header_index_map.find(key);
            if (it != fetch_header_index_map.end()) {
                switch (it->second) {
                    case Fetch_header::Date:
                        current_email.body.header.date = value;
                        break;
                    case Fetch_header::From:
                        current_email.body.header.from = value;
                        break;
                    case Fetch_header::Sender:
                        current_email.body.header.sender.push_back(value);
                        break;
                    case Fetch_header::Reply_To:
                        current_email.body.header.reply_to.push_back(value);
                        break;
                    case Fetch_header::To:
                        current_email.body.header.to.push_back(value);
                        break;
                    case Fetch_header::Cc:
                        current_email.body.header.cc.push_back(value);
                        break;
                    case Fetch_header::Bcc:
                        current_email.body.header.bcc.push_back(value);
                        break;
                    case Fetch_header::Message_ID:
                        current_email.body.header.message_id.push_back(value);
                        break;
                    case Fetch_header::In_Reply_To:
                        current_email.body.header.in_reply_to.push_back(value);
                        break;
                    case Fetch_header::References:
                        current_email.body.header.references.push_back(value);
                        break;
                    case Fetch_header::Subject:
                        current_email.body.header.subject.push_back(value);
                        break;
                    case Fetch_header::Comments:
                        current_email.body.header.comments.push_back(value);
                        break;
                    case Fetch_header::Keywords:
                        current_email.body.header.keywords.push_back(value);
                        break;
                    case Fetch_header::Resent_Date:
                        current_email.body.header.resent_date.push_back(value);
                        break;
                    case Fetch_header::Resent_From:
                        current_email.body.header.resent_from.push_back(value);
                        break;
                    case Fetch_header::Resent_Sender:
                        current_email.body.header.resent_sender.push_back(value);
                        break;
                    case Fetch_header::Resent_To:
                        current_email.body.header.resent_to.push_back(value);
                        break;
                    case Fetch_header::Resent_Cc:
                        current_email.body.header.resent_cc.push_back(value);
                        break;
                    case Fetch_header::Resent_Bcc:
                        current_email.body.header.resent_bcc.push_back(value);
                        break;
                    case Fetch_header::Resent_Message_ID:
                        current_email.body.header.resent_message_id.push_back(value);
                        break;
                    case Fetch_header::Return_Path:
                        current_email.body.header.return_path.push_back(value);
                        break;
                    case Fetch_header::Received:
                        current_email.body.header.received.push_back(value);
                        break;
                }
            } else {
                // 处理可选头部字段
                current_email.body.header.optional[key].push_back(value);
            }
            
            // 移动到下一个字段
            left = right + 2;
            
            // 检查是否到达头部结束（空行）
            if (left + 2 <= str.size() && str.substr(left, 2) == "\r\n") {
                right = left + 2;
                break;
            }
        }
    }
    
    // 处理正文
    if (has_text == true) {
        if (right + 2 < str.size()) {
            current_email.body.text = str.substr(right + 2);
        }
    }
    
    return 0;
}

} // namespace flow_table
