1.Message结构体,包含元素有:
    - string 类型的 tag
    - string 类型的 command
    - vector<string> 类型的 args
    - vector<Email> 类型的 fetch

2.Email结构体,包含元素有(IMAP 协议中的邮件结构):
    - Mr.zhao
    (在实现的时候只定义一下,记录一个 TODO)

3.FourTuple 结构体,包含元素有:
    - string 类型的 sourceIP
    - int 类型的 sourcePort
    - string 类型的 destIP
    - int 类型的 destPort

4.CircularString类,功能为:
    - 一个固定大小的字符串缓冲区
    - 可以在末尾添加字符串
    - 可以查找第n次出现的字符串
    - 可以获取子字符串
    - 可以删除k及之前的所有元素

5.InputPacket 结构体,包含元素有:
    - string 类型的 payload
    - string 类型的 type
    - FourTuple 类型的 fourTuple
    - string 类型的 源角色(C2S/S2C)
    - string 类型的 状态位(需要关心是否是传输阶段 0x22)

6.Flow 类
    包含元素有:
    - FourTuple 类型的 c2sTuple
    - FourTuple 类型的 s2cTuple
    - vector<Message> 类型的 c2sMessages
    - vector<Message> 类型的 s2cMessages
    - vector<string> 类型的 c2sState
    - vector<string> 类型的 s2cState
    - CircularString 类型的 c2sBuffer (大小要大于最大的邮件,可以通过配置文件确定,,,或者用动态扩容)
    - CircularString 类型的 s2cBuffer (大小要大于最大的邮件,可以通过配置文件确定)
    包含方法有:
    - 调用 c2sParse 解析 c2sBuffer,更新 c2sState,更新 c2sMessages
    - 调用 s2cParse 解析 s2cBuffer,更新 s2cState,更新 s2cMessages
    - 输出 c2sMessages 和 s2cMessages
    - 删除流(在 logout 后)

7.HashFlowTable 类
    包含元素有:
    - HashMap<int, Flow*> 类型的 flowMap
    包含方法有:
    - 创建新流
    - 向已经存在的 Flow 的 Buffer 中添加 payload
    - 删除流(在超时后)

关闭的时候:
1.关闭(0x21)的时候先判断:
    流是否被删除
    若没被删除,则关闭