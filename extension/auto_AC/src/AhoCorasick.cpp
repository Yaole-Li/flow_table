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
     auto current = root;
    for (auto &wd : utf8word)
    {
        if (!current->children.count(wd))
        {
            current->children[wd] = new TrieNode();
        }
        current = current->children[wd];
    }
    current->matched = true;
    // 记录敏感词
    current->words = utf8word;
}
// 为Trie树构建失配指针;
// 如果节点i的失败指针指向j，那么root到j的的字符串是root到i的字符串的一个后缀
void AhoCorasick::buildFailPointer()
{
    /*1.将root的下一级孩子节点的fail指针全部指向为root
    2.将节点入队进行bfs遍历
    3.先建立失败指针再入队
    */
    std::queue<TrieNode*> q;
    for (auto &entry : root->children)
    {
        entry.second->fail = root;
        q.push(entry.second);
    }
    while (!q.empty())
    {
        auto current = q.front();
        q.pop();
        for (auto &entry : current->children)
        {
            char byte = entry.first;
            auto child = entry.second;
            auto move_fail = current->fail;
            while (move_fail != root && !move_fail->children.count(byte))
            {
                move_fail = move_fail->fail;
            }
            if (move_fail->children.count(byte))
            {
                child->fail = move_fail->children[byte];
            }
            else
            {
                child->fail = root;
            }
            q.push(child);
        }
    }
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
void AhoCorasick::splitWord(const std::string &words,std::vector<std::string>&characters){
        int i = 0;
        while(i<words.size()){
            //英文和标点就是1字节长度，也就是说一个字符的字节长度最短为1
            int charactersize = 1;
            //如果是中文
            if(words[i] & 0x80){
            unsigned char character = words[i];
            character<<=1;
                while(character&0x80){
                    charactersize++;
                    character<<=1;
                }
            }
        std::string word = words.substr(i,charactersize);
        characters.push_back(word);
            i += charactersize;
        }
}
void AhoCorasick::queryWord( std::string str)
{
    auto node =root;
    std::vector<std::string>words;
    splitWord(str,words);
    int i =0;
    while(i<words.size()){
        std::string word = words[i];
        bool matchFound = true;
        for(int j=0;j<word.size();j++){
            unsigned char byte = word[j];
            while(node!=root&&!node->children.count(byte)){
                node=node->fail;
            }
            if(node->children.count(byte)){
                node = node->children[byte];     
            }
            else{
                //这个字没有匹配成功
                matchFound = false;
                break; 
            }
        }
        if(matchFound&&node->matched){
            std::cout << "敏感词: " << node->words << " 匹配到位置: " << i << std::endl;
        }
        ++i;
    }
}

