#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <set>
#include <fstream>
#include <sstream>
#include <chrono>
#include "../include/AhoCorasick.h"

AhoCorasick::AhoCorasick()
{
    root = new TrieNode();
    root->fail = root;
}
AhoCorasick::~AhoCorasick()
{
    clear(root);
}

// 用敏感词构建Trie树
void AhoCorasick::buildTrie(const std::string &utf8word)
{
    auto node = root;
    std::vector<std::string> characters;
    splitWord(utf8word, characters);
    for (const auto &ch : characters)
    {
        if (!node->children.count(ch))
        {
            node->children[ch] = new TrieNode();
        }
        node = node->children[ch];
    }
    node->matched = true;
    node->words.push_back(utf8word);
    std::cout << "[buildTrie] 插入敏感词: " << utf8word << std::endl;
}
// 为Trie树构建失配指针;
// 如果节点i的失败指针指向j，那么root到j的的字符串是root到i的字符串的一个后缀
void AhoCorasick::buildFailPointer()
{
    std::queue<TrieNode *> q;
    root->fail = root;
    for (auto &p : root->children)
    {
        p.second->fail = root;
        q.push(p.second);
    }
    int nodeCount = 1; // root
    while (!q.empty())
    {
        TrieNode *cur = q.front();
        q.pop();
        nodeCount++;
        for (auto &p : cur->children)
        {
            const std::string &ch = p.first;
            TrieNode *child = p.second;
            TrieNode *fail = cur->fail;
            // debug: 打印当前节点信息和起始 fail 指针
            std::cout << "[buildFailPointer DEBUG] 处理节点, cur 地址: " << cur << ", 起始 fail 地址: " << fail << ", cur->words: ";
            if (cur->words.empty()) std::cout << "<empty>";
            else for (const auto &w : cur->words) std::cout << w << ",";
            std::cout << std::endl;
            while (fail != root && !fail->children.count(ch))
            {
                std::cout << "[buildFailPointer DEBUG] 循环中, fail 地址: " << fail << ", 无法匹配 ch=" << ch << std::endl;
                fail = fail->fail;
            }
            if (fail->children.count(ch) && fail->children[ch] != child)
            {
                std::cout << "[buildFailPointer DEBUG] 匹配失败跳转节点, 地址: " << fail->children[ch] << std::endl;
                child->fail = fail->children[ch];
            }
            else
            {
                std::cout << "[buildFailPointer DEBUG] 未匹配到, 将 fail 设置为 root" << std::endl;
                child->fail = root;
            }
            if (child->fail->matched)
            {
                child->matched = true;
                for (const auto &w : child->fail->words) {
                    child->words.push_back(w);
                }
            }
            q.push(child);
            std::cout << "[buildFailPointer] 节点fail指针建立: ch=" << ch << std::endl;
        }
    }
    std::cout << "[buildFailPointer] 节点总数: " << nodeCount << std::endl;
}

// 释放内存空间
void AhoCorasick::clear(TrieNode *node)
{
    std::queue<TrieNode*> q;
    q.push(node);
    while (!q.empty())
    {
        auto current_node = q.front();
        q.pop();
        for (auto &entry : current_node->children)
        {
            q.push(entry.second);
        }
        delete current_node;
    }
}
//将字节拆分字符串并记录字符位置
/*
    每个汉字三个字节，其中的第一个字节的前三位都为1,因此按位与来确定这个汉字的位置
*/
void AhoCorasick::splitWord(const std::string &words,std::vector<std::string>&characters)
{
    int i=0;
    while(i<words.size()){
        unsigned char c=words[i];
        int charactersize=1;
        if((c&0x80)==0){
            // ascii
        }
        else if((c&0xE0)==0xC0){
            charactersize=2;
        }
        else if((c&0xF0)==0xE0){
            charactersize=3;
        }
        else if((c&0xF8)==0xF0){
            charactersize=4;
        }
        std::string word = words.substr(i,charactersize);
        characters.push_back(word);
        i += charactersize;
    }
    std::cout << "[splitWord] 分词结果: ";
    for (const auto& w : characters) {
        std::cout << w << "|";
    }
    std::cout << std::endl;
}
void AhoCorasick::queryWord(const std::string &str)
{
    std::vector<std::string> words;
    splitWord(str, words);
    std::cout << "[queryWord] 输入分词数: " << words.size() << std::endl;
    auto node = root;
    for (int i = 0; i < words.size(); ++i) {
        const std::string &ch = words[i];
        while (node != root && !node->children.count(ch)) {
            node = node->fail;
            std::cout << "[queryWord] fail跳转, ch=" << ch << std::endl;
        }
        if (node->children.count(ch)) {
            node = node->children[ch];
            std::cout << "[queryWord] 状态转移, ch=" << ch << std::endl;
        } else {
            std::cout << "[queryWord] 状态转移失败, ch=" << ch << std::endl;
        }
        if (node->matched) {
            for (const auto& w : node->words) {
                std::cout << "[queryWord] 敏感词: " << w << " 匹配到位置: " << i << std::endl;
            }
        }
    }
}
