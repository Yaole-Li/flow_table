#ifndef CIRCULAR_STRING_H
#define CIRCULAR_STRING_H

#include <vector>
#include <string>
#include <stdexcept>

/**
 * @brief 环形字符串类，用于高效管理固定容量的字符缓冲区
 * 
 * 该类实现了一个环形缓冲区，当缓冲区满时，新添加的元素会覆盖最旧的元素。
 * 主要用于需要保留最近N个字符的场景，如日志记录、数据流处理等。
 */
class CircularString {
private:
    std::vector<char> buffer;  // 底层存储
    size_t capacity;           // 缓冲区总容量
    size_t head;               // 逻辑起始位置（物理索引）
    size_t count;              // 当前有效元素数量

    // 将逻辑索引转换为物理索引
    size_t physical_index(size_t logical_index) const;

public:
    /**
     * @brief 构造函数，创建指定容量的环形字符串
     * @param size 环形缓冲区的容量
     * @throw std::invalid_argument 如果容量为0
     */
    explicit CircularString(size_t size);

    /**
     * @brief 在末尾插入字符串
     * @param str 要插入的字符串
     */
    void push_back(const std::string& str);

    /**
     * @brief 查找第n次出现的字符串（从1开始计数）
     * @param target 要查找的字符串
     * @param n 第几次出现
     * @return 目标字符串第n次出现的首字符逻辑索引
     * @throw std::out_of_range 如果字符串没有出现足够次数
     */
    size_t find_nth(const std::string& target, size_t n) const;

    /**
     * @brief 获取子字符串[m, n]
     * @param m 起始索引（包含）
     * @param n 结束索引（包含）
     * @return 指定范围的子字符串
     * @throw std::out_of_range 如果索引范围无效
     */
    std::string substring(size_t m, size_t n) const;

    /**
     * @brief 删除k及之前的所有元素
     * @param k 要删除的最后一个元素的索引
     * @throw std::out_of_range 如果索引超出范围
     */
    void erase_up_to(size_t k);

    /**
     * @brief 获取当前有效元素数量
     * @return 有效元素数量
     */
    size_t size() const noexcept;

    /**
     * @brief 获取缓冲区总容量
     * @return 缓冲区容量
     */
    size_t cap() const noexcept;
};

#endif // CIRCULAR_STRING_H