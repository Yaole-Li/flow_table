# 哈希流表设计图

## 类关系图

```mermaid
classDiagram
    class FourTuple {
        +string sourceIP  %% 源IP地址
        +int sourcePort   %% 源端口
        +string destIP    %% 目标IP地址
        +int destPort     %% 目标端口
    }
    
    class FlowType {
        <<enumeration>>
        C2S  %% 客户端到服务器
        S2C  %% 服务器到客户端
    }
    
    class InputPacket {
        +FourTuple fourTuple  %% 四元组信息
        +FlowType flowType    %% 流类型
        +byte[] payload       %% 数据负载
    }
    
    class HashFlowTable {
        -HashMap~int, Flow~ flowMap            %% 哈希流表，键为哈希值，值为流对象
        -int hashFourTuple(FourTuple)          %% 计算四元组的哈希值
        -int hashString(string)                %% 计算字符串哈希值
        +HashFlowTable()                       %% 构造函数
        +Flow* getOrCreateFlow(FourTuple, FlowType)  %% 获取或创建流
        +void processPacket(InputPacket)       %% 处理输入数据包
        +void processC2SFlow(Flow*, byte[])    %% 处理C2S流
        +void processS2CFlow(Flow*, byte[])    %% 处理S2C流
        +void outputResults()                  %% 输出处理结果
    }
    
    class Flow {
        -FourTuple tuple               %% 流的四元组标识
        -FlowType type                 %% 流类型
        -vector~Message~ messages      %% 解析出的邮件消息列表
        -string state                  %% IMAP协议状态，如A0001等
        +Flow(FourTuple, FlowType)     %% 构造函数
        +void addMessage(Message)      %% 添加邮件消息
        +string getState()             %% 获取当前状态
        +void setState(string)         %% 设置新状态
        +void updateState(string, string) %% 更新状态（TODO）
        +void generateOutput()         %% 生成输出
    }
    
    class ImapParser {
        +static void parseC2SData(Flow*, byte[])              %% 解析C2S数据
        +static void parseS2CData(Flow*, byte[])              %% 解析S2C数据
        -static Message extractMessageFromC2SData(byte[])     %% 从C2S数据中提取消息
        -static Message extractMessageFromS2CData(byte[])     %% 从S2C数据中提取消息
        -static string extractCommand(byte[])                 %% 从载荷中提取IMAP命令
        -static string extractResponse(byte[])                %% 从载荷中提取IMAP响应
    }
    
    class Message {
        -string subject    %% 邮件主题
        -string content    %% 邮件内容
        -string from       %% 发件人
        -string to         %% 收件人
        +string toString() %% 转换为字符串输出
    }
    
    InputPacket --> FourTuple : 包含四元组
    InputPacket --> FlowType : 包含流类型
    HashFlowTable o-- Flow : 管理多个流
    HashFlowTable --> ImapParser : 调用解析器
    HashFlowTable ..> InputPacket : 处理数据包
    Flow --> FourTuple : 包含四元组
    Flow --> FlowType : 包含流类型
    Flow o-- Message : 包含多个消息
    ImapParser ..> Message : 创建消息
    ImapParser --> Flow : 更新流和状态
```

## 处理流程图

```mermaid
flowchart TD
    start[开始] --> create[创建哈希流表实例]
    create --> input[接收输入数据包]
    input --> hash[计算四元组哈希值]
    hash --> check{检查流是否存在?}
    check -->|是| get[获取已有流]
    check -->|否| new[创建新流]
    new --> type
    get --> type{流类型?}
    type -->|C2S| c2s[调用C2S IMAP解析]
    type -->|S2C| s2c[调用S2C IMAP解析]
    c2s --> extract1[提取消息内容]
    s2c --> extract2[提取消息内容]
    extract1 --> add1[添加消息到流对象]
    extract2 --> add2[添加消息到流对象]
    add1 --> state1[提取命令并更新流状态]
    add2 --> state2[提取响应并更新流状态]
    state1 --> more{还有更多数据包?}
    state2 --> more
    more -->|是| input
    more -->|否| output[输出所有流中的邮件消息]
    output --> finish[结束]
```

## 数据流向图

```mermaid
flowchart LR
    input[输入数据包\nInputPacket] --> hash[哈希流表\nHashFlowTable]
    hash --> flow[流对象\nFlow]
    flow --> parser[IMAP解析器\nImapParser]
    parser --> msg[邮件消息\nMessage]
    parser --> state[状态更新]
    state --> flow
    msg --> flow
    flow --> output[输出邮件结果]
    
    subgraph C2S流
    c2sData[客户端命令数据] --> c2sCmd[提取命令] --> c2sParse[C2S数据解析]
    end
    
    subgraph S2C流
    s2cData[服务器响应数据] --> s2cResp[提取响应] --> s2cParse[S2C数据解析]
    end
    
    c2sParse --> parser
    s2cParse --> parser