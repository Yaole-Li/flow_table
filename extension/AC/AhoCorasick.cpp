#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <set>
#include <fstream>
#include <sstream>
#include <chrono>
// 读取文件
std::string readFileToutf8(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "无法打开文件" << filename << std::endl;
        return 0;
    }
    // 将文件内容读入字符串
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

/*树节点定义*/
struct Trienode
{
    std::map<unsigned char, Trienode *> children; // 孩子节点
    Trienode *fail;                               // 失配指针
    bool matched;                                 // 终止位
    std::string words;                            // 敏感词 直接输出
    Trienode() : fail(nullptr), matched(false), words("") {};
};
class AhoCorasick
{
public:
    Trienode *root;
    AhoCorasick()
    {
        root = new Trienode();
        root->fail = root;
    }
    ~AhoCorasick()
    {
        clear(root);
    }

    // 用敏感词构建Trie树
    void bulidTrie(const std::string &utf8word)
    {
        auto current = root;
        for (auto &wd : utf8word)
        {
            if (!current->children.count(wd))
            {
                current->children[wd] = new Trienode();
            }
            current = current->children[wd];
        }
        current->matched = true;
        // 记录敏感词
        current->words = utf8word;
    }
    // 为Trie树构建失配指针;
    // 如果节点i的失败指针指向j，那么root到j的的字符串是root到i的字符串的一个后缀
    void buildFailPointer()
    {
        /*1.将root的下一级孩子节点的fail指针全部指向为root
          2.将节点入队进行bfs遍历
          3.先建立失败指针再入队
        */
        std::queue<Trienode *> q;
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
        std::cout << "失配指针构建完成" << std::endl;
    }

    // 释放内存空间
    void clear(Trienode *node)
    {
        std::queue<Trienode *> q;
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
    void spliteWord(const std::string &words,std::vector<std::string>&characters){
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
    void queryword(std::string str)
    {
        auto node =root;
        std::vector<std::string>words;
        spliteWord(str,words);
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
};

int main()
{

    std::ifstream file("/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/AC/test.txt");
    if (!file)
    {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }
    auto ac = AhoCorasick();
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            std::cout << line.size();
            ac.bulidTrie(line);
        }
    }
    std::cout << "trie树构建完成" << std::endl;
    ac.buildFailPointer();
    file.close();
    // 2. 读取邮件内容
    std::string content = readFileToutf8("/Users/liyaole/Documents/works/c_work/imap_works/flow_table/extension/AC/input.txt");
    if(content.empty()){
        return 1;
    }
    // std::string str = "woshi你好哈哈来你好我是大强asdkfjalsdfjlajdf安科技发达卢卡斯的积分卡萨丁积分卡地方";
    // // 获取起始时间
    auto start = std::chrono::high_resolution_clock::now();
    ac.queryword(content);
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续的时间，以毫秒为单位
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "耗时 " << duration.count() << " 毫秒" << std::endl;
    //std::cout << ac.root->children.size() << std::endl;
    return 0;
}