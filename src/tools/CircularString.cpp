#include "../../include/tools/CircularString.h"
#include <algorithm>
#include <unordered_map>

// 将逻辑索引转换为物理索引
size_t CircularString::physical_index(size_t logical_index) const {
    return (head + logical_index) % capacity;
}

// 构造函数，创建指定容量的环形字符串
CircularString::CircularString(size_t size) 
    : capacity(size), head(0), count(0) {
    if (capacity == 0) {
        throw std::invalid_argument("Capacity must be positive");
    }
    buffer.resize(capacity);
}

// 在末尾插入字符串
void CircularString::push_back(const std::string& str) {
    for (char c : str) {
        // 计算实际要插入的物理位置
        size_t insert_pos = physical_index(count);
        
        if (count < capacity) {
            // 缓冲区未满，直接追加
            buffer[insert_pos] = c;
            ++count;
        } else {
            // 缓冲区已满，覆盖最旧元素并移动头指针
            buffer[insert_pos] = c;
            head = (head + 1) % capacity; // 最旧元素被覆盖，头指针向前移动
        }
    }
}

// 查找第n次出现的字符串（Sunday算法优化）
size_t CircularString::find_nth(const std::string& target, size_t n) const {
    if (target.empty()) {
        throw std::invalid_argument("查找目标不能为空字符串");
    }
    
    const size_t target_len = target.length();
    const size_t buffer_len = count;
    
    if (target_len > buffer_len) {
        throw std::out_of_range("字符串未找到足够次数");
    }
    
    // Sunday算法预处理坏字符表
    std::unordered_map<char, size_t> bad_char_shift;
    for (size_t i = 0; i < target_len; ++i) {
        bad_char_shift[target[i]] = target_len - i;
    }
    
    size_t occurrences = 0;
    size_t current = 0;
    
    while (current <= buffer_len - target_len) {
        size_t j;
        // 检查匹配
        for (j = 0; j < target_len; ++j) {
            const size_t phys_idx = physical_index(current + j);
            if (buffer[phys_idx] != target[j]) break;
        }
        
        if (j == target_len) {
            if (++occurrences == n) {
                return current;
            }
            current += 1; // 找到匹配后移动1位继续查找
        } else {
            // 计算跳跃步长
            const size_t next_char_pos = current + target_len;
            if (next_char_pos >= buffer_len) break;
            
            const char next_char = buffer[physical_index(next_char_pos)];
            current += bad_char_shift.count(next_char) ? 
                      bad_char_shift[next_char] : 
                      target_len + 1;
        }
    }
    
    throw std::out_of_range("字符串未找到足够次数");
}

// 获取子字符串[m, n]
std::string CircularString::substring(size_t m, size_t n) const {
    if (m > n || n >= count) {
        throw std::out_of_range("Invalid index range");
    }

    std::string result;
    result.reserve(n - m + 1);
    for (size_t i = m; i <= n; ++i) {
        result.push_back(buffer[physical_index(i)]);
    }
    return result;
}

// 删除k及之前的所有元素
void CircularString::erase_up_to(size_t k) {
    if (k >= count) {
        throw std::out_of_range("Erase index out of range");
    }

    // 计算新的头位置和剩余元素数量
    head = physical_index(k + 1); // 新的头是k+1对应的物理位置
    count -= (k + 1);
}

// 获取当前有效元素数量
size_t CircularString::size() const noexcept {
    return count;
}

// 获取缓冲区总容量
size_t CircularString::cap() const noexcept {
    return capacity;
}