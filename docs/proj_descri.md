本项目需要根据输入的一个结构体,构建基于四元组的 HASH 流表,最后输出邮件

已知输入的结构体中有:
- 四元组
- 流类型(C2S/S2C)
- 载荷数据

总处理流程:
1. 建立 Hash 流表类
2. 根据四元组建立流表中的流
3. 根据流的类型(C2S/S2C),调用对应的 IMAP 解析方法
4. 将解析出来的内容进行输出

TODO:
1. 查看 IMAP 中 email 字段定义,完善 email 类型定义(zhao)
2. 写请求的 Parser (zhao)
3. 编写 main 函数,实现总处理流程(li)
4. 完善其他的结构体(li)

```c
// 四元组结构体定义
struct FourTuple {
    string sourceIP;
    int sourcePort;
    string destIP;
    int destPort;
};

// 流类型枚举
enum FlowType {
    C2S,  // 客户端到服务器
    S2C   // 服务器到客户端
};

// 输入结构体定义
struct InputPacket {
    FourTuple fourTuple;  // 四元组
    FlowType flowType;    // 流类型
    byte[] payload;       // 载荷数据
};

// 哈希流表类设计
class HashFlowTable {
private:
    // 使用哈希表存储流，键为四元组的哈希值，值为Flow对象
    HashMap<int, Flow> flowMap;
    
    // 计算四元组的哈希值
    int hashFourTuple(FourTuple tuple) {
        // 简单的哈希算法，实际实现可能更复杂
        int hash = 0;
        hash = hash * 31 + hashString(tuple.sourceIP);
        hash = hash * 31 + tuple.sourcePort;
        hash = hash * 31 + hashString(tuple.destIP);
        hash = hash * 31 + tuple.destPort;
        return hash;
    }
    
    // 辅助函数：计算字符串哈希值
    int hashString(string str) {
        // 字符串哈希算法
        int hash = 0;
        for (char c : str) {
            hash = hash * 31 + c;
        }
        return hash;
    }

public:
    // 构造函数
    HashFlowTable() {
        // 初始化哈希表
    }
    
    // 根据四元组查找或创建流
    Flow* getOrCreateFlow(FourTuple tuple) {
        int hash = hashFourTuple(tuple);
        
        // 检查流是否已存在
        if (flowMap.contains(hash)) {
            return &flowMap[hash];
        }
        
        // 如果不存在，创建新流
        Flow newFlow(tuple);
        flowMap[hash] = newFlow;
        return &flowMap[hash];
    }
    
    // 处理输入包
    void processPacket(InputPacket packet) {
        // 获取或创建流
        Flow* flow = getOrCreateFlow(packet.fourTuple);
        
        // 根据流类型选择处理方法
        if (packet.flowType == FlowType.C2S) {
            processC2SFlow(flow, packet.payload);
        } else if (packet.flowType == FlowType.S2C) {
            processS2CFlow(flow, packet.payload);
        }
    }
    
    // 处理客户端到服务器的流
    void processC2SFlow(Flow* flow, byte[] payload) {
        // 调用IMAP C2S解析方法
        ImapParser.parseC2SData(flow, payload);
    }
    
    // 处理服务器到客户端的流
    void processS2CFlow(Flow* flow, byte[] payload) {
        // 调用IMAP S2C解析方法
        ImapParser.parseS2CData(flow, payload);
    }
    
    // 输出处理结果
    void outputResults() {
        // 遍历所有流并输出处理结果
        for (auto& [hash, flow] : flowMap) {
            // 生成邮件输出
            flow.generateOutput();
        }
    }
};

// 流类定义 - 修改后同时包含两个方向
class Flow {
private:
    FourTuple tuple;               // 流的四元组标识
    vector<Message> c2sMessages;   // C2S方向解析出的邮件消息
    vector<Message> s2cMessages;   // S2C方向解析出的邮件消息
    string c2sState;               // C2S方向的IMAP协议状态
    string s2cState;               // S2C方向的IMAP协议状态
    CircularString c2sBuffer;      // C2S方向的数据缓冲区
    CircularString s2cBuffer;      // S2C方向的数据缓冲区
    
public:
    // 构造函数
    Flow(FourTuple t) {
        tuple = t;
        c2sState = "";            // 初始化C2S状态为空
        s2cState = "";            // 初始化S2C状态为空
        c2sBuffer = CircularString(10240);  // 为每个方向创建10KB的环形缓冲区
        s2cBuffer = CircularString(10240);
    }
    
    // 添加C2S方向解析出的消息
    void addC2SMessage(Message msg) {
        c2sMessages.push_back(msg);
    }
    
    // 添加S2C方向解析出的消息
    void addS2CMessage(Message msg) {
        s2cMessages.push_back(msg);
    }
    
    // 获取C2S方向当前状态
    string getC2SState() {
        return c2sState;
    }
    
    // 获取S2C方向当前状态
    string getS2CState() {
        return s2cState;
    }
    
    // 设置C2S方向新状态
    void setC2SState(string newState) {
        c2sState = newState;
    }
    
    // 设置S2C方向新状态
    void setS2CState(string newState) {
        s2cState = newState;
    }
    
    // 向C2S缓冲区添加数据
    void addC2SData(byte[] data) {
        c2sBuffer.push_back(string(data, data.length));
    }
    
    // 向S2C缓冲区添加数据
    void addS2CData(byte[] data) {
        s2cBuffer.push_back(string(data, data.length));
    }
    
    // 获取C2S缓冲区
    CircularString* getC2SBuffer() {
        return &c2sBuffer;
    }
    
    // 获取S2C缓冲区
    CircularString* getS2CBuffer() {
        return &s2cBuffer;
    }
    
    // 更新C2S状态
    void updateC2SState(string command) {
        // 根据IMAP协议实现C2S方向状态转换逻辑
    }
    
    // 更新S2C状态
    void updateS2CState(string response) {
        // 根据IMAP协议实现S2C方向状态转换逻辑
    }
    
    // 生成输出 - 合并两个方向的消息
    void generateOutput() {
        // 将解析出的所有邮件内容输出
        cout << "C2S方向发现的邮件:" << endl;
        for (Message msg : c2sMessages) {
            cout << "  " << msg.toString() << endl;
        }
        
        cout << "S2C方向发现的邮件:" << endl;
        for (Message msg : s2cMessages) {
            cout << "  " << msg.toString() << endl;
        }
    }
};

// IMAP解析器（假设实现）
class ImapParser {
public:
    // 解析客户端到服务器的数据
    static void parseC2SData(Flow* flow, byte[] payload) {
        // 将数据添加到C2S缓冲区
        flow->addC2SData(payload);
        
        // 解析IMAP客户端命令
        // 例如：LOGIN, SELECT, FETCH等
        Message message = extractMessageFromC2SData(flow->getC2SBuffer());
        if (message != null) {
            flow->addC2SMessage(message);
        }
        
        // 提取命令并更新C2S流状态
        string command = extractCommand(flow->getC2SBuffer());
        flow->updateC2SState(command);
    }
    
    // 解析服务器到客户端的数据
    static void parseS2CData(Flow* flow, byte[] payload) {
        // 将数据添加到S2C缓冲区
        flow->addS2CData(payload);
        
        // 解析IMAP服务器响应
        // 例如：邮件数据、状态响应等
        Message message = extractMessageFromS2CData(flow->getS2CBuffer());
        if (message != null) {
            flow->addS2CMessage(message);
        }
        
        // 提取响应并更新S2C流状态
        string response = extractResponse(flow->getS2CBuffer());
        flow->updateS2CState(response);
    }
    
private:
    // 从C2S数据中提取邮件消息
    static Message extractMessageFromC2SData(CircularString* buffer) {
        // 实现IMAP客户端命令的解析逻辑
        return new Message();
    }
    
    // 从S2C数据中提取邮件消息
    static Message extractMessageFromS2CData(CircularString* buffer) {
        // 实现IMAP服务器响应的解析逻辑
        return new Message();
    }
    
    // 从C2S缓冲区提取命令
    static string extractCommand(CircularString* buffer) {
        // 实现命令提取逻辑
        return "";
    }
    
    // 从S2C缓冲区提取响应
    static string extractResponse(CircularString* buffer) {
        // 实现响应提取逻辑
        return "";
    }
};

// 消息类定义
class Message {
private:
    string subject;
    string content;
    string from;
    string to;
    
public:
    // 构造函数和其他方法
    
    string toString() {
        return "From: " + from + "\nTo: " + to + "\nSubject: " + subject;
    }
};

// main函数：模拟处理流程
int main() {
    // 1. 创建哈希流表实例
    HashFlowTable flowTable;
    
    // 2. 模拟输入数据包
    InputPacket packet1;
    packet1.fourTuple = {
        "192.168.1.100",
        12345,
        "10.0.0.1",
        143  // IMAP端口
    };
    packet1.flowType = FlowType.C2S;
    packet1.payload = /* 客户端IMAP命令数据 */;
    
    InputPacket packet2;
    packet2.fourTuple = {
        "10.0.0.1",
        143,  // IMAP端口
        "192.168.1.100",
        12345
    };
    packet2.flowType = FlowType.S2C;
    packet2.payload = /* 服务器IMAP响应数据 */;
    
    // 3. 处理数据包
    flowTable.processPacket(packet1);
    flowTable.processPacket(packet2);
    
    // 4. 输出结果
    flowTable.outputResults();
    
    return 0;
}
