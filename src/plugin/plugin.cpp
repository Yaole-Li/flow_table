/**
 * @file plugin.cpp
 * @brief IMAP流量分析和关键词检测插件接口实现
 * 
 * 实现了plugin.h中声明的四个接口函数：
 * - Create：插件创建（全局初始化）
 * - Single：插件构建（线程初始化）
 * - Filter：数据过滤处理
 * - Remove：插件拆除（资源清理）
 */

#include "../../include/plugin/plugin.h"

// 全局变量

// 内部全局变量
static flow_table::HashFlowTable* flowTable = nullptr;
// 关键词检测相关全局变量
static AhoCorasick* acDetector = nullptr;
static flow_table::ConfigParser* configPtr = nullptr;
static std::string projectRoot;
// 全局配置文件路径
static std::string configFilePath;

// 获取当前目录的工具函数
std::string getCurrentDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return std::string(cwd) + "/";
    }
    // 返回默认路径
    std::cerr << "警告: 无法获取当前目录，使用相对路径" << std::endl;
    return "./";
}

// 获取项目根目录的工具函数 - 保留以向后兼容
std::string getProjectRoot() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string currentPath(cwd);
        // 查找flow_table目录
        size_t pos = currentPath.find("flow_table");
        if (pos != std::string::npos) {
            // 返回到flow_table目录的路径
            return currentPath.substr(0, pos + 10) + "/"; // 10是"flow_table"的长度
        }
    }
    // 返回默认路径
    std::cerr << "警告: 无法确定项目根目录，使用相对路径" << std::endl;
    return "./";
}

// 判断文件是否存在
bool fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

// 初始化关键词检测器
bool initKeywordDetector() {
    std::cout << "初始化关键词检测器..." << std::endl;
    
    // 从配置文件获取敏感词典路径
    std::string dictionaryPath = configPtr->getString("Paths.keyword_dict", 
        projectRoot + "/extension/auto_AC/file/sensitive.txt");
    
    // 验证文件存在性
    if (!fileExists(dictionaryPath)) {
        std::cerr << "词典文件不存在: " << dictionaryPath << std::endl;
        return false;
    }
    
    std::cout << "使用词典文件: " << dictionaryPath << std::endl;
    
    // 创建AC自动机
    acDetector = new AhoCorasick();
    
    // 读取敏感词库并构建Trie树
    std::ifstream dictFile(dictionaryPath);
    if (!dictFile.is_open()) {
        std::cerr << "打开词典文件失败" << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(dictFile, line)) {
        if (!line.empty()) {
            // 处理CRLF换行符
            if (line.back() == '\r') {
                line.pop_back();
            }
            acDetector->buildTrie(line);
        }
    }
    dictFile.close();
    
    std::cout << "Trie树构建完成" << std::endl;
    acDetector->buildFailPointer();
    std::cout << "失败指针构建完成" << std::endl;
    
    return true;
}

// 执行关键词检测
void performKeywordDetection(const std::string& content) {
    if (!acDetector) {
        std::cerr << "错误: 关键词检测器未初始化" << std::endl;
        return;
    }
    
    std::cout << "\n===== 执行关键词检测 =====" << std::endl;
    std::cout << "内容长度: " << content.length() << " 字节" << std::endl;
    
    // 执行检测
    acDetector->queryWord(content);
}

// ------------------------------ 1. 插件创建（全局初始化） ------------------------------
// 该部分只在程序启动时执行一次
int Create(unsigned short Version, unsigned short Amount, const char *Option) {
    // 使用参数以避免警告
    std::cout << "插件创建: 版本" << Version << ", 参数数量" << Amount << ", 选项" << (Option ? Option : "无") << std::endl;
    std::cout << "执行全局初始化..." << std::endl;
    
    // 获取项目根目录
    projectRoot = getProjectRoot();
    
    // 初始化配置解析器
    configPtr = new flow_table::ConfigParser();
    
    // 尝试加载配置文件，按以下顺序尝试：
    // 1. 使用命令行指定的配置文件路径（如果有）
    // 2. 尝试当前目录下的config.ini
    // 3. 尝试项目根目录下的config.ini
    bool configLoaded = false;
    
    if (!configFilePath.empty()) {
        // 使用命令行指定的配置文件
        std::cout << "尝试加载指定的配置文件: " << configFilePath << std::endl;
        configLoaded = configPtr->loadFromFile(configFilePath);
    }
    
    if (!configLoaded) {
        // 尝试当前目录
        std::string currentDirConfig = getCurrentDir() + "config.ini";
        std::cout << "尝试加载当前目录下的配置文件: " << currentDirConfig << std::endl;
        configLoaded = configPtr->loadFromFile(currentDirConfig);
    }
    
    if (!configLoaded) {
        // 尝试项目根目录
        std::string projectRootConfig = projectRoot + "config.ini";
        std::cout << "尝试加载项目根目录下的配置文件: " << projectRootConfig << std::endl;
        configLoaded = configPtr->loadFromFile(projectRootConfig);
    }
    
    if (!configLoaded) {
        std::cerr << "警告: 无法加载任何配置文件，将使用默认值" << std::endl;
    } else {
        std::cout << "成功加载配置文件" << std::endl;
    }
    
    // 初始化关键词检测器
    if (!initKeywordDetector()) {
        std::cerr << "警告: 关键词检测器初始化失败" << std::endl;
    }
    
    std::cout << "全局初始化完成" << std::endl;
    
    return 0; // 成功返回0
}

// ------------------------------ 2. 插件构建（线程初始化） ------------------------------
// 该部分在每个工作线程启动时执行
int Single(unsigned short Thread, const char *Option) {
    // 使用参数以避免警告
    std::cout << "线程初始化: 线程编号" << Thread << ", 选项" << (Option ? Option : "无") << std::endl;
    std::cout << "执行线程初始化..." << std::endl;
    
    // 创建哈希流表（每个线程一个实例）
    flowTable = new flow_table::HashFlowTable();
    
    // 从配置文件中读取流超时时间（配置文件中的Flow.flow_timeout参数）
    // 默认值为120000毫秒（120秒）
    int64_t flowTimeoutMs = 120000; // 默认值
    
    if (configPtr) {
        flowTimeoutMs = configPtr->getInt64("Flow.flow_timeout", 120000);
        std::cout << "从配置文件读取流超时时间: " << flowTimeoutMs << " 毫秒" << std::endl;
    } else {
        std::cout << "使用默认流超时时间: 120000 毫秒" << std::endl;
    }
    
    // 设置流超时时间
    flowTable->setFlowTimeout(flowTimeoutMs);
    
    std::cout << "线程初始化完成" << std::endl;
    
    return 0; // 成功返回0
}

// ------------------------------ 3. Filter 处理函数 ------------------------------
// 数据过滤函数，处理每个数据包
int Filter(TASK *Import, TASK **Export) {
    // 置动作为通告
    *Export = Import;

    switch(Import->Inform) {
        case 0X12: {
            // 设置动作为转发
            (*Export)->Action = 0X22;

            // 2. 根据 TASK 中的源端和宿端创建四元组
            FourTuple fourTuple;
            
            // 判断数据包方向
            bool isC2S = (Import->Source.Role == 'C');
            
            // 设置源IP和端口 - 始终保持C2S方向
            if (isC2S) {
                // C2S方向，正常设置
                // 设置源IP（客户端）
                if (Import->Source.IPvN == 4) {
                    fourTuple.srcIPvN = 4;
                    fourTuple.srcIPv4 = Import->Source.IPv4;
                } else if (Import->Source.IPvN == 6) {
                    fourTuple.srcIPvN = 6;
                    memcpy(fourTuple.srcIPv6, Import->Source.IPv6, 16);
                }
                fourTuple.sourcePort = Import->Source.Port;
                
                // 设置目标IP（服务器）
                if (Import->Target.IPvN == 4) {
                    fourTuple.dstIPvN = 4;
                    fourTuple.dstIPv4 = Import->Target.IPv4;
                } else if (Import->Target.IPvN == 6) {
                    fourTuple.dstIPvN = 6;
                    memcpy(fourTuple.dstIPv6, Import->Target.IPv6, 16);
                }
                fourTuple.destPort = Import->Target.Port;
            } else {
                // S2C方向，反转方向使其成为C2S方向
                // 设置源IP（客户端，即Target）
                if (Import->Target.IPvN == 4) {
                    fourTuple.srcIPvN = 4;
                    fourTuple.srcIPv4 = Import->Target.IPv4;
                } else if (Import->Target.IPvN == 6) {
                    fourTuple.srcIPvN = 6;
                    memcpy(fourTuple.srcIPv6, Import->Target.IPv6, 16);
                }
                fourTuple.sourcePort = Import->Target.Port;
                
                // 设置目标IP（服务器，即Source）
                if (Import->Source.IPvN == 4) {
                    fourTuple.dstIPvN = 4;
                    fourTuple.dstIPv4 = Import->Source.IPv4;
                } else if (Import->Source.IPvN == 6) {
                    fourTuple.dstIPvN = 6;
                    memcpy(fourTuple.dstIPv6, Import->Source.IPv6, 16);
                }
                fourTuple.destPort = Import->Source.Port;
            }
            
            // 3. 创建InputPacket，只包含数据、类型和四元组
            flow_table::InputPacket packet;
            
            // 设置数据包的有效载荷
            std::string payload(reinterpret_cast<char*>(Import->Buffer), Import->Length);
            packet.payload = payload;
            
            // 设置数据包类型（即源角色）
            packet.type = isC2S ? "C2S" : "S2C";
            
            // 设置四元组 - 现在四元组始终是C2S方向
            packet.fourTuple = fourTuple;
            
            // 4. 处理数据包
            bool processed = flowTable->processPacket(packet);
            
            // 5. 输出处理结果并进行关键词检测
            if (processed) {
                // 改变思路：截获输出结果并对其进行关键词检测
                // 重定向cout到字符串流
                std::stringstream outputStream;
                std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
                std::cout.rdbuf(outputStream.rdbuf());
                
                // 输出处理结果到字符串流
                flowTable->outputResults();
                
                // 恢复标准输出
                std::cout.rdbuf(oldCoutStreamBuf);
                
                // 获取捕获的输出内容
                std::string outputContent = outputStream.str();
                
                // 将捕获的输出内容写入到标准输出
                std::cout << outputContent;
                
                // 如果是S2C数据包，分析输出内容中的邮件并进行关键词检测
                if (!isC2S) {
                    std::cout << "检测到S2C数据包，准备分析输出内容..." << std::endl;
                    
                    // 在输出内容中查找并检测邮件内容
                    if (!outputContent.empty()) {
                        // 通过输出内容中的邮件标识词来提取邮件
                        // 例如，输出内容中通常包含"EMAIL:"、"Subject:"、"From:"等标识
                        
                        std::cout << "对解析后的输出内容进行关键词检测..." << std::endl;
                        performKeywordDetection(outputContent);
                        
                        /*
                        // 同时也对原始S2C数据包进行检测，以防有些内容未被输出
                        std::string rawContent = std::string(reinterpret_cast<char*>(Import->Buffer), Import->Length);
                        if (!rawContent.empty()) {
                            std::cout << "对原始S2C数据包内容进行关键词检测..." << std::endl;
                            performKeywordDetection(rawContent);
                        }
                        */
                    }
                }
            }

            break;
        }
        default:
            (*Export)->Action = 0X21;
            break;
    }

    return 0;
}

// ------------------------------ 4. 插件拆除（资源清理） ------------------------------
// 负责资源释放和清理
void Remove() {
    std::cout << "执行清理..." << std::endl;
    
    // 清理所有资源
    if (flowTable != nullptr) {
        // 获取所有流对象并输出最终结果
        flowTable->outputResults();
        
        // 删除哈希流表
        // 用析构函数
        delete flowTable;
        flowTable = nullptr;
    }
    
    // 清理关键词检测器
    if (acDetector != nullptr) {
        delete acDetector;
        acDetector = nullptr;
    }
    
    // 清理配置解析器
    if (configPtr != nullptr) {
        delete configPtr;
        configPtr = nullptr;
    }
    
    std::cout << "清理完成" << std::endl;
}

// 提供一个函数用于外部设置配置文件路径
void SetConfigFilePath(const char* path) {
    if (path != nullptr) {
        configFilePath = path;
    }
}
