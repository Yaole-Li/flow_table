#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace flow_table {

/**
 * @brief 简单的INI配置文件解析器
 */
class ConfigParser {
public:
    /**
     * @brief 默认构造函数
     */
    ConfigParser() = default;

    /**
     * @brief 从文件加载配置
     * @param filename 配置文件路径
     * @return 是否成功加载
     */
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            // 去除行首尾的空格
            line = trim(line);

            // 跳过空行和注释行
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            // 解析节名称 [section]
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // 解析键值对
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiterPos));
                std::string value = line.substr(delimiterPos + 1);
                
                // 处理行内注释，如果有分号或#号，截取前面的部分作为值
                size_t commentPos = value.find_first_of(";#");
                if (commentPos != std::string::npos) {
                    value = value.substr(0, commentPos);
                }
                
                // 去除值的首尾空格
                value = trim(value);

                // 如果有节，则使用section.key作为键
                if (!currentSection.empty()) {
                    key = currentSection + "." + key;
                }

                config[key] = value;
            }
        }

        file.close();
        return true;
    }

    /**
     * @brief 获取字符串值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = config.find(key);
        return (it != config.end()) ? it->second : defaultValue;
    }

    /**
     * @brief 获取整数值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = config.find(key);
        if (it != config.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 获取64位整数值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    int64_t getInt64(const std::string& key, int64_t defaultValue = 0) const {
        auto it = config.find(key);
        if (it != config.end()) {
            try {
                return std::stoll(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 获取双精度浮点值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = config.find(key);
        if (it != config.end()) {
            try {
                return std::stod(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 获取布尔值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = config.find(key);
        if (it != config.end()) {
            std::string value = toLower(it->second);
            return (value == "true" || value == "yes" || value == "1" || value == "on");
        }
        return defaultValue;
    }

    /**
     * @brief 设置配置值
     * @param key 键名
     * @param value 配置值
     */
    void setValue(const std::string& key, const std::string& value) {
        config[key] = value;
    }

    /**
     * @brief 保存配置到文件
     * @param filename 配置文件路径
     * @return 是否成功保存
     */
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        // 构建节映射
        std::map<std::string, std::map<std::string, std::string>> sections;
        std::map<std::string, std::string> noSection;

        for (const auto& item : config) {
            size_t dotPos = item.first.find('.');
            if (dotPos != std::string::npos) {
                std::string section = item.first.substr(0, dotPos);
                std::string key = item.first.substr(dotPos + 1);
                sections[section][key] = item.second;
            } else {
                noSection[item.first] = item.second;
            }
        }

        // 先写入无节的配置
        for (const auto& item : noSection) {
            file << item.first << " = " << item.second << std::endl;
        }

        // 写入各节的配置
        for (const auto& section : sections) {
            file << std::endl << "[" << section.first << "]" << std::endl;
            for (const auto& item : section.second) {
                file << item.first << " = " << item.second << std::endl;
            }
        }

        file.close();
        return true;
    }

private:
    std::map<std::string, std::string> config;

    // 工具函数：去除字符串首尾空格
    std::string trim(const std::string& str) const {
        const std::string whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    // 工具函数：转为小写
    std::string toLower(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), 
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }
};

} // namespace flow_table

#endif // CONFIG_PARSER_H
