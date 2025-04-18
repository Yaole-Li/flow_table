#ifndef AHOCORASICK_H
#define AHOCORASICK_H

#include <map>
#include <queue>
#include <string>
#include <vector>

// Trie 节点结构
struct TrieNode {
    std::map<std::string, TrieNode*> children; // 子节点映射，按UTF-8字符为单位
    TrieNode* fail;                              // 失配指针
    bool matched;                                // 是否匹配终止
    std::vector<std::string> words;              // 存储敏感词（若为终止节点）

    TrieNode() : fail(nullptr), matched(false), words() {}
};

// Aho-Corasick 自动机类
class AhoCorasick {
public:
    AhoCorasick();
    ~AhoCorasick();

    // 构建 Trie 树
    void buildTrie(const std::string& utf8word);

    // 构建失配指针
    void buildFailPointer();

    // 查询敏感词
    void queryWord(const std::string& str);

private:
    TrieNode* root;

    // 释放 Trie 树内存
    void clear(TrieNode* node);

    // 拆分字符串为字符（支持多字节字符如中文）
    void splitWord(const std::string& words, std::vector<std::string>& characters);
};

#endif // AHOCORASICK_H