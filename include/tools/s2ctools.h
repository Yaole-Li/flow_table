#ifndef S2CTOOLS_H
#define S2CTOOLS_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include "../tools/types.h"
#include "../tools/CircularString.h"

namespace flow_table {

/**
 * 解析IMAP邮件正文内容
 * @param str 待解析的字符串
 * @param current_email 当前邮件对象，用于存储解析结果
 * @param has_header 是否解析头部
 * @param has_text 是否解析正文
 * @return 解析结果，0表示成功，负数表示失败
 */
int Resolve_imap_body(std::string& str, Email& current_email, bool has_header, bool has_text);

/**
 * Base64解码函数
 * @param input 输入的Base64编码字符串
 * @return 解码后的字符串
 */
std::string Base64_decode(const std::string& input);

/**
 * UTF8转GBK编码
 * @param str UTF8编码的字符串
 * @return GBK编码的字符串
 */
std::string Utf8_to_gbk(const std::string& str);

/**
 * GBK转UTF8编码
 * @param str GBK编码的字符串
 * @return UTF8编码的字符串
 */
std::string Gbk_to_utf8(const std::string& str);

#if defined(__linux__) || defined(__GNUC__)
/**
 * 编码转换函数
 * @param charsetSrc 源字符集
 * @param charsetDest 目标字符集
 * @param inbuf 输入缓冲区
 * @param inSz 输入大小
 * @param outbuf 输出缓冲区
 * @param outSz 输出大小
 * @return 转换结果，0表示成功，负数表示失败
 */
int Encoding_convert(const char* charsetSrc, const char* charsetDest, char* inbuf,
    size_t inSz, char* outbuf, size_t outSz);
#endif

} // namespace flow_table

#endif // S2CTOOLS_H
