#include "../../include/flows/flow_manager.h"
#include "../../include/tools/types.h"
#include "../../include/tools/s2ctools.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <cstring>

namespace flow_table {

// 实现Flow::parseS2CData方法
bool Flow::parseS2CData() {
    // 这里将Resolve_imap_body函数的实现搬运过来，并做适当修改
    Email currentEmail;
    size_t emailCount = 0;
    Message currentMessage;    // 存储当前响应信息的结构体
    std::string temp;          // 用于暂存解析参数时遇到的字符串
    size_t index = 0;          // 下一个要从缓冲区中读取的字符的位置
    int leftBracketCount = 0;  // 用于解析参数时的括号匹配
    char currentChar = 0;      // 当前正在被解析的字符
    size_t currentNumber = 0;  // 用于解析数字
    size_t endLineIndex;       // 行尾索引
    
    // 这里添加Reslove_s2c函数的主体实现，并将cstring替换为s2cBuffer
    while (1) {
        index = 0;  // 索引重新指向缓冲区起点
        currentEmail = Email();    // 清空Email结构体中的数据
        try {
            // 判断响应类型
            currentChar = s2cBuffer.at(index++);
            // + 代表 Command Continuation Request，目前直接无视
            if (currentChar == '+') {
                throw std::runtime_error("目前暂时不处理+开头的响应");
                continue;
            }
            // * 代表单方向响应，目前只关注FETCH响应
            else if (currentChar == '*') {
                currentChar = s2cBuffer.at(index++);
                // 删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (currentChar != ' ' && currentChar != 9) {
                    throw std::runtime_error("*后面没有找到空白符");
                    continue;
                }
                do {
                    currentChar = s2cBuffer.at(index++);
                } while (currentChar == ' ' || currentChar == 9);
                // Fetch响应包的第二项是一个数字，代表邮件的序号
                if (!std::isdigit(currentChar)) {
                    throw std::runtime_error("这不是一个Fetch响应包");
                    continue;
                }
                currentNumber = 0;
                do {
                    currentNumber = currentNumber * 10 + currentChar - '0';
                    currentChar = s2cBuffer.at(index++);
                } while (std::isdigit(currentChar));
                currentEmail.sequence_number = currentNumber;
                // 删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (currentChar != ' ' && currentChar != 9) {
                    throw std::runtime_error("这不是一个Fetch响应包！");
                    continue;
                }
                do {
                    currentChar = s2cBuffer.at(index++);
                } while (currentChar == ' ' || currentChar == 9);
                // 判断这个字段是不是FETCH
                temp.clear();
                if (currentChar >= 33 && currentChar <= 126) {
                    temp.push_back(std::toupper(currentChar));
                    currentChar = s2cBuffer.at(index++);
                    while (currentChar >= 33 && currentChar <= 126) {
                        temp.push_back(std::toupper(currentChar));
                        currentChar = s2cBuffer.at(index++);
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
                // 删除空白符（空格和水平制表符）,且至少得有一个空白符
                if (currentChar != ' ' && currentChar != 9) {
                    throw std::runtime_error("FETCH 后面没有找到空白符");
                    continue;
                }
                do {
                    currentChar = s2cBuffer.at(index++);
                } while (currentChar == ' ' || currentChar == 9);
                //
                // 读取键值对
                //
                if (currentChar != '(') {
                    throw std::runtime_error("Fetch后没找到左括号");
                }
                currentChar = std::toupper(s2cBuffer.at(index++));
                // 循环读取键值对，直到遇到右括号
                while (currentChar != ')') {
                    temp = "";  // 准备读取键
                    if (currentChar >= 33 && currentChar <= 126) {
                        do {
                            if (currentChar == '(') {
                                temp.push_back(currentChar);
                                currentChar = std::toupper(s2cBuffer.at(index++));
                                leftBracketCount = 1;
                                while (leftBracketCount > 0) {
                                    if (currentChar >= 32 && currentChar <= 126) {
                                        if (currentChar == '(') {
                                            leftBracketCount++;
                                        }
                                        else if (currentChar == ')') {
                                            leftBracketCount--;
                                        }
                                        temp.push_back(currentChar);
                                        currentChar = std::toupper(s2cBuffer.at(index++));
                                    }
                                    else {
                                        throw std::runtime_error("键中发现不可打印字符");
                                    }
                                }
                            }
                            else if(currentChar == '[') {
                                temp.push_back(currentChar);
                                currentChar = std::toupper(s2cBuffer.at(index++));
                                leftBracketCount = 1;
                                while (leftBracketCount > 0) {
                                    if (currentChar >= 32 && currentChar <= 126) {
                                        if (currentChar == '[') {
                                            leftBracketCount++;
                                        }
                                        else if (currentChar == ']') {
                                            leftBracketCount--;
                                        }
                                        temp.push_back(currentChar);
                                        currentChar = std::toupper(s2cBuffer.at(index++));
                                    }
                                    else {
                                        throw std::runtime_error("键中发现不可打印字符");
                                    }
                                }
                            }
                            else {
                                temp.push_back(currentChar);
                                currentChar = std::toupper(s2cBuffer.at(index++));
                            }
                        } while (currentChar >= 33 && currentChar <= 126);
                    }
                    else {
                        throw std::runtime_error("Fetch没找到键的字符");
                    }
                    // 删除空白符（空格和水平制表符）,且至少得有一个空白符
                    if (currentChar != ' ' && currentChar != 9) {
                        throw std::runtime_error("键的后面没有找到空白符");
                        continue;
                    }
                    do {
                        currentChar = s2cBuffer.at(index++);
                    } while (currentChar == ' ' || currentChar == 9);
                    // 根据不同的键，采用不同的方式读取值
                    auto it = fetch_name_index_map.find(temp);
                    if (it != fetch_name_index_map.end()) {
                        
                        switch (it->second) {
                        case Fetch_name::BODYSTRUCTURE:
                            // 假设bodystructure的值在一对括号内
                            if (currentChar != '(') {
                                throw std::runtime_error("BODYSTRUCTURE值的首个字符不是左括号");
                            }
                            currentEmail.bodystructure.push_back(currentChar);
                            currentChar = s2cBuffer.at(index++);
                            leftBracketCount = 1;
                            while (leftBracketCount > 0) {
                                if (currentChar >= 32 && currentChar <= 126) {
                                    if (currentChar == '(') {
                                        leftBracketCount++;
                                    }
                                    else if (currentChar == ')') {
                                        leftBracketCount--;
                                    }
                                    currentEmail.bodystructure.push_back(currentChar);
                                    currentChar = s2cBuffer.at(index++);
                                }
                                else {
                                    throw std::runtime_error("BODYSTRUCTURE值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::ENVELOPE:
                            // 假设envelope的值在一对括号内
                            if (currentChar != '(') {
                                throw std::runtime_error("ENVELOPE值的首个字符不是左括号");
                            }
                            currentEmail.envelope.push_back(currentChar);
                            currentChar = s2cBuffer.at(index++);
                            leftBracketCount = 1;
                            while (leftBracketCount > 0) {
                                if (currentChar >= 32 && currentChar <= 126) {
                                    if (currentChar == '(') {
                                        leftBracketCount++;
                                    }
                                    else if (currentChar == ')') {
                                        leftBracketCount--;
                                    }
                                    currentEmail.envelope.push_back(currentChar);
                                    currentChar = s2cBuffer.at(index++);
                                }
                                else {
                                    throw std::runtime_error("ENVELOPE值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::FLAGS:
                            // 假设flags的值在一对括号内
                            if (currentChar != '(') {
                                throw std::runtime_error("FLAGS值的首个字符不是左括号");
                            }
                            currentEmail.flags.push_back(currentChar);
                            currentChar = s2cBuffer.at(index++);
                            leftBracketCount = 1;
                            while (leftBracketCount > 0) {
                                if (currentChar >= 32 && currentChar <= 126) {
                                    if (currentChar == '(') {
                                        leftBracketCount++;
                                    }
                                    else if (currentChar == ')') {
                                        leftBracketCount--;
                                    }
                                    currentEmail.flags.push_back(currentChar);
                                    currentChar = s2cBuffer.at(index++);
                                }
                                else {
                                    throw std::runtime_error("FLAGS值内发现不可打印字符");
                                }
                            }
                            break;
                        case  Fetch_name::INTERNALDATE:
                            // 假设INTERNALDATE的值在一对双引号内
                            if (currentChar != '\"') {
                                throw std::runtime_error("FLAGS值的首个字符不是双引号");
                            }
                            currentChar = s2cBuffer.at(index++);
                            while (currentChar != '\"') {
                                if (currentChar >= 32 && currentChar <= 126) {
                                    currentEmail.internaldate.push_back(currentChar);
                                    currentChar = s2cBuffer.at(index++);
                                }
                                else {
                                    throw std::runtime_error("FLAGS值内发现不可打印字符");
                                }
                            }
                            currentChar = s2cBuffer.at(index++);
                            break;
                        case  Fetch_name::RFC822:  // 等同于BODY[]
                            // 假设RFC822的值是literal字符串的格式
                            if (currentChar != '{') {
                                throw std::runtime_error("RFC822的值不是以'{'开头");
                            }
                            currentChar = s2cBuffer.at(index++);
                            // 计算RFC822的字节数
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("RFC822的值的'{'后不是数字");
                                continue;
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            if (currentChar != '}') {
                                throw std::runtime_error("RFC822的值的\"{数字\"后不是'}'");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\r') {
                                throw std::runtime_error("RFC822的值的\"{数字}\"后不是换行");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\n') {
                                throw std::runtime_error("RFC822的值的\"{数字}\"换行后不是回车");
                            }
                            s2cBuffer.at(index + currentNumber - 1); // 试一下读取字符串末尾会不会越界，以免白忙活
                            currentChar = s2cBuffer.at(index++);
                            // 先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < currentNumber;i++) {
                                temp.push_back(currentChar);
                                currentChar = s2cBuffer.at(index++);
                            }
                            Resolve_imap_body(temp, currentEmail, true, true);
                            break;
                        case  Fetch_name::RFC822_HEADER:  // 等同于BODY.PEEK[HEADER]
                            // 假设RFC822.HEADER的值是literal字符串的格式
                            if (currentChar != '{') {
                                throw std::runtime_error("RFC822.HEADER的值不是以'{'开头");
                            }
                            currentChar = s2cBuffer.at(index++);
                            // 计算RFC822.HEADER的字节数
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("RFC822.HEADER的值的'{'后不是数字");
                                continue;
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            if (currentChar != '}') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字\"后不是'}'");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\r') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字}\"后不是换行");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\n') {
                                throw std::runtime_error("RFC822.HEADER的值的\"{数字}\"换行后不是回车");
                            }
                            s2cBuffer.at(index + currentNumber - 1); // 试一下读取字符串末尾会不会越界，以免白忙活
                            currentChar = s2cBuffer.at(index++);
                            // 先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < currentNumber;i++) {
                                temp.push_back(currentChar);
                                currentChar = s2cBuffer.at(index++);
                            }
                            // 先处理头部信息
                            Resolve_imap_body(temp, currentEmail, true, false);
                            break;
                        case  Fetch_name::RFC822_SIZE:
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("RFC822_SIZE的值不是数字");
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            currentEmail.rfc822_size = currentNumber;
                            break;
                        case  Fetch_name::RFC822_TEXT:  // 等同于BODY[TEXT]
                            // 假设RFC822.TEXT的值是literal字符串的格式
                            if (currentChar != '{') {
                                throw std::runtime_error("RFC822.TEXT的值不是以'{'开头");
                            }
                            currentChar = s2cBuffer.at(index++);
                            // 计算RFC822.TEXT的字节数
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("RFC822.TEXT的值的'{'后不是数字");
                                continue;
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            if (currentChar != '}') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字\"后不是'}'");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\r') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字}\"后不是换行");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\n') {
                                throw std::runtime_error("RFC822.TEXT的值的\"{数字}\"换行后不是回车");
                            }
                            s2cBuffer.at(index + currentNumber - 1); // 试一下读取字符串末尾会不会越界，以免白忙活
                            currentChar = s2cBuffer.at(index++);
                            currentEmail.body.text = "";
                            // 字符串里的内容都是正文
                            for (int i = 0;i < currentNumber;i++) {
                                currentEmail.body.text.push_back(currentChar);
                                currentChar = s2cBuffer.at(index++);
                            }
                            break;
                        case  Fetch_name::UID:
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("UID的值不是数字");
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            currentEmail.uid = (std::uint32_t)(currentNumber);
                            break;
                        }// End switch
                        // 找到下一个非空白符（空格和水平制表符）
                        while (currentChar == ' ' || currentChar == 9) {
                            currentChar = s2cBuffer.at(index++);
                        }
                    }// End if
                    else {
                        if (temp.compare(0,5,"BODY[") != 0) {
                            throw std::runtime_error("Fetch响应中含有未知的键");
                        }
                        // 到这里就已经确定键是BODY[...]或者BODY[...]<>的格式了
                        else {
                            bool has_header = false;
                            bool has_text = false;
                            // 假设Header的名称中都不包含括号，也就是BODY[...]中只可能有HEADER.FILEDS (...) 这一对括号
                            size_t left_bracket_index = temp.find('(',5);
                            size_t right_bracket_index = temp.find(')',5);
                            // 在BODY[...]的中括号内，圆括号外找有没有HEADER字段和TEXT字段
                            if (left_bracket_index != std::string::npos && right_bracket_index != std::string::npos) {
                                has_header = temp.find("HEADER") > left_bracket_index ? has_header : true;
                                if (has_header == false) {
                                    has_text = temp.find("TEXT") > left_bracket_index ? has_text : true;
                                }
                            }
                            else {
                                size_t right_square_bracket_index = temp.find(']',5);
                                if (right_square_bracket_index != std::string::npos) {
                                    has_header = temp.find("HEADER",5,right_square_bracket_index - 5) != std::string::npos;
                                    has_text = temp.find("TEXT",5,right_square_bracket_index - 5) != std::string::npos;
                                }
                            }
                            if (has_header == false && has_text == false) {
                                has_header = true;
                                has_text = true;
                            }
                            // 假设BODY的值是literal字符串的格式
                            if (currentChar != '{') {
                                throw std::runtime_error("BODY的值不是以'{'开头");
                            }
                            currentChar = s2cBuffer.at(index++);
                            // 计算BODY的字节数
                            if (!std::isdigit(currentChar)) {
                                throw std::runtime_error("BODY的值的'{'后不是数字");
                                continue;
                            }
                            currentNumber = 0;
                            do {
                                currentNumber = currentNumber * 10 + currentChar - '0';
                                currentChar = s2cBuffer.at(index++);
                            } while (std::isdigit(currentChar));
                            if (currentChar != '}') {
                                throw std::runtime_error("BODY的值的\"{数字\"后不是'}'");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\r') {
                                throw std::runtime_error("BODY的值的\"{数字}\"后不是换行");
                            }
                            currentChar = s2cBuffer.at(index++);
                            if (currentChar != '\n') {
                                throw std::runtime_error("BODY的值的\"{数字}\"换行后不是回车");
                            }
                            s2cBuffer.at(index + currentNumber - 1); // 试一下读取字符串末尾会不会越界，以免白忙活
                            currentChar = s2cBuffer.at(index++);
                            // 先把邮件体中的内容存下来，之后再分析
                            temp = "";
                            for (int i = 0;i < currentNumber;i++) {
                                temp.push_back(currentChar);
                                currentChar = s2cBuffer.at(index++);
                            }
                            Resolve_imap_body(temp, currentEmail, has_header, has_text);
                            // 找到下一个非空白符（空格和水平制表符）
                            while (currentChar == ' ' || currentChar == 9) {
                                currentChar = s2cBuffer.at(index++);
                            }
                        }
                    }
                }
                // 找回车换行
                endLineIndex = s2cBuffer.find(index, s2cBuffer.size(), '\r');
                if (endLineIndex == (size_t)(-1)) {
                    throw std::out_of_range("Fetch的右括号后没找到换行符");
                }
                if (s2cBuffer.at(endLineIndex + 1) != '\n') {
                    throw std::runtime_error("回车换行不匹配");
                }
                
                // 试着对邮件内容进行解码
                if (currentEmail.body.text != "") {
                    auto it4 = currentEmail.body.header.optional.find("Content-Type");
                    if (it4 != currentEmail.body.header.optional.end()) {
                        if (it4->second[0].find("text/plain") != std::string::npos) {
                            currentEmail.body.text = Base64_decode(currentEmail.body.text);
                            if (it4->second[0].find("charset = \"gb18030\"") != std::string::npos) {
                                currentEmail.body.text = Gbk_to_utf8(currentEmail.body.text);
                            }
                        }
                        
                    }
                }

                // 做一些收尾操作，然后continue读取下一封邮件
                currentMessage.fetch.push_back(currentEmail);
                emailCount++;
                s2cBuffer.erase_up_to(endLineIndex + 1);
                std::cout << "成功完成一封邮件的解析" << std::endl;
                continue;
            }
            // 正常的响应类型，结构为 <Tag> <Status> <text>
            else if (currentChar >= 33 && currentChar <= 126) {
                // 解析标签的第一个字节
                currentMessage.tag.push_back(currentChar);
                currentChar = s2cBuffer.at(index++);
                // 解析标签的剩余字节
                while (currentChar >= 33 && currentChar <= 126) {
                    currentMessage.tag.push_back(currentChar);
                    currentChar = s2cBuffer.at(index++);
                }
                // 删除标签和状态回复之间的空白符,且至少得有一个空白符
                if (currentChar != ' ' && currentChar != 9) {
                    throw std::runtime_error("没有找到标签和状态回复中间的空白符");
                    continue;
                }
                do {
                    currentChar = s2cBuffer.at(index++);
                } while (currentChar == ' ' || currentChar == 9);
                // 解析状态回复（"OK" "NO" "BAD"）的第一个字节
                currentChar = std::toupper(currentChar);
                if (currentChar == 'O') {
                    currentMessage.command.push_back(currentChar);
                    currentChar = std::toupper(s2cBuffer.at(index++));
                    if (currentChar == 'K') {
                        currentMessage.command.push_back(currentChar);
                        currentChar = s2cBuffer.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else if (currentChar == 'N' && currentChar != 'B') {
                    currentMessage.command.push_back(currentChar);
                    currentChar = std::toupper(s2cBuffer.at(index++));
                    if (currentChar == 'O') {
                        currentMessage.command.push_back(currentChar);
                        currentChar = s2cBuffer.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else if (currentChar == 'B') {
                    currentMessage.command.push_back(currentChar);
                    currentChar = std::toupper(s2cBuffer.at(index++));
                    if (currentChar == 'A') {
                        currentMessage.command.push_back(currentChar);
                        currentChar = std::toupper(s2cBuffer.at(index++));
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                    if (currentChar == 'D') {
                        currentMessage.command.push_back(currentChar);
                        currentChar = s2cBuffer.at(index++);
                    }
                    else {
                        throw std::runtime_error("状态回复不合法");
                    }
                }
                else {
                    throw std::runtime_error("状态回复不合法");
                }
                if (currentChar >= 33 && currentChar <= 126) {
                    throw std::runtime_error("状态回复不合法");
                }
                // 删除状态回复和文本之间的空白符（空格和水平制表符）
                while (currentChar == ' ' || currentChar == 9) {
                    currentChar = s2cBuffer.at(index++);
                }
                // 找到行末尾的换行符
                endLineIndex = s2cBuffer.find(index - 1, s2cBuffer.size(), '\r');
                if (endLineIndex == (size_t)(-1)) {
                    throw std::out_of_range("缓冲区内未找到换行符");
                }
                if (s2cBuffer.at(endLineIndex + 1) != '\n') {
                    throw std::runtime_error("回车换行不匹配");
                }
                // 把到换行符为止的字符一股脑存进args[0]中
                temp = "";
                while (index <= endLineIndex) {
                    temp.push_back(currentChar);
                    currentChar = s2cBuffer.at(index++);
                }
                currentMessage.args.push_back(temp);
                s2cMessages.push_back(currentMessage);     // 保存message
                // 清理缓冲区
                s2cBuffer.erase_up_to(endLineIndex + 1);
                break;
            }
            // 以上三种情况都不满足说明这根本不是一个合法的响应
            else {
                throw std::out_of_range("非法响应格式");
                continue;
            }
        }
        catch (const std::out_of_range e) {
            if (emailCount > 0) {
                s2cMessages.push_back(currentMessage);
            }
            return -1;
        }
        catch (const std::runtime_error e) {
            // 找到行末尾的换行符
            endLineIndex = s2cBuffer.find((index - 1 > 0 ? index - 1 : 0), s2cBuffer.size(), '\r');
            if (endLineIndex == (size_t)(-1)) {
                // 情况一：响应语句不合法，试图删除整行语句时却发现buffer里压根找不到换行符
                return -1;  // 表达buffer里的数据不够用
            }
            try {
                if (s2cBuffer.at(endLineIndex + 1) != '\n') {
                    // 情况二：响应语句不合法，能在buffer里找到换行符，但下一个字节不是回车
                    std::cerr << e.what() << ':';
                    for (int i = 0;i < endLineIndex;i++) {
                        if (s2cBuffer.at(i) >= 32 && s2cBuffer.at(i) <= 126) {
                            std::cerr << s2cBuffer.at(i);
                        }
                        else {
                            std::cerr << "\\0x" << std::setw(2) << std::setfill('0') << std::hex << int(s2cBuffer.at(i));
                        }
                    }
                    std::cerr << std::endl << std::dec;
                    s2cBuffer.erase_up_to(endLineIndex);     // 清理缓冲区
                }
                else {
                    // 情况三：响应语句不合法，能在buffer里找到换行符，且下一个字节是回车
                    std::cerr << e.what() << ':';
                    for (int i = 0;i < endLineIndex;i++) {
                        if (s2cBuffer.at(i) >= 32 && s2cBuffer.at(i) <= 126) {
                            std::cerr << s2cBuffer.at(i);
                        }
                        else {
                            std::cerr << "\\0x" << std::setw(2) << std::setfill('0') << std::hex << int(s2cBuffer.at(i));
                        }
                    }
                    std::cerr << std::endl << std::dec;
                    s2cBuffer.erase_up_to(endLineIndex + 1);     // 清理缓冲区
                }
            }
            catch (std::out_of_range e) {
                // 情况四：响应语句不合法，能在buffer里找到换行符，但由于换行符在缓冲区结尾所以不知道下一个字符是不是回车
                return -1; // 表达buffer里的数据不够用
            }
            continue;
        }
    }
    return 0;
}

} // namespace flow_table