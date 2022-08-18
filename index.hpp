#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include "util.hpp"
#include "log.hpp"

namespace ns_index
{
    struct DocInfo
    {
        std::string title;
        std::string content;
        std::string url;
        uint64_t doc_id;
    };

    struct InvertedElem
    {
        uint64_t doc_id;
        std::string word;
        int weight;
    };

    //倒排拉链
    typedef std::vector<InvertedElem> InvertedList;

    class Index
    {
    private:
        std::vector<DocInfo> forward_index;//正排索引
        std::unordered_map<std::string, InvertedList> inverted_index;//倒排索引
    private:
        //设计为单例模式
        static Index* instance;
        static std::mutex mtx;
    private:
        Index(){}
        Index(const Index&) = delete;
        Index& operator=(const Index&) = delete;
    public:
        ~Index(){}
    public:
        static Index* GetInstance()
        {
            if(nullptr == instance)
            {
                mtx.lock();
                if(nullptr == instance)
                {
                    instance = new Index();
                }
                mtx.unlock();
            }

            return instance;
        }

        //根据doc_id找到文档内容
        DocInfo* GetForwardIndex(uint64_t doc_id)
        {
            if(doc_id >= forward_index.size())
            {
                std::cerr << "doc_id out of range" << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }

        //根据关键字word获得倒排拉链
        InvertedList* GetInvertedList(const std::string& word)
        {
            if(inverted_index.find(word) == inverted_index.end())
            {
                std::cerr << word << " have not exist" << std::endl;
                return nullptr;
            }
            return &inverted_index[word];
        }

        //根据去标签，格式化之后的文档，构建正排和倒排索引
        bool BuildIndex(const std::string& input)//获取parser处理完后的数据
        {
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if(!in.is_open())
            {
                std::cerr << "open file " << input << " failed!" << std::endl;
                return false;
            }

            std::string line;
            int cnt = 0;
            while(std::getline(in, line))
            {
                DocInfo* doc = BuildForwardIndex(line);
                if(nullptr == doc)
                {
                    std::cerr << "build " << line << " error!" << std::endl;
                    continue;
                }
                
                BuildInvertedIndex(*doc);
                //for debug
                ++cnt;
                if(cnt % 500 == 0)
                    LOG(NORMAL, "当前已建立的索引文档: " + std::to_string(cnt));
            }

            return true;
        }
    private:
        DocInfo* BuildForwardIndex(const std::string& line)
        {
            //1.解析line，字符串切分
            //line -> 3个string : title, content, url
            std::vector<std::string> results;
            const std::string sep = "\3";
            ns_util::StringUtil::CutString(line, &results, sep);
            if(results.size() != 3)
            {
                return nullptr;
            }

            //2.字符串填充到DocInfo
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id =  forward_index.size();

            //3.插入到正排索引的vector
            forward_index.push_back(std::move(doc));

            return &forward_index.back();
        }

        void BuildInvertedIndex(const DocInfo& doc)
        {
            //word -> 倒排拉链
            //保存word分别在title,content中出现的次数
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;

                word_cnt()
                    :title_cnt(0), content_cnt(0)
                {}
            };

            //1.分别对title和content进行分词，并统计词频
            //将word与出现的次数建立映射
            std::unordered_map<std::string, word_cnt> word_cnt_map;

            //对title进行分词
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::WordSegmentation(doc.title, &title_words);
            
            //词频统计
            for(std::string s : title_words)
            {
                boost::to_lower(s);//将我们的分词统一转化为小写
                ++word_cnt_map[s].title_cnt;
            }

            //对content进行分词
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::WordSegmentation(doc.content, &content_words);
            
            //词频统计
            for(std::string s : content_words)
            {
                boost::to_lower(s);//将我们的分词统一转化为小写
                ++word_cnt_map[s].content_cnt;
            }

            //2.构建倒排拉链
            //自定义title和content中出现的词所占的权重
            const int title_wight = 10;
            const int content_weight = 1;
            //构建倒排元素
            for(auto& word_pair : word_cnt_map)
            {
                InvertedElem item;
                item.doc_id = doc.doc_id;
                item.word = word_pair.first;
                item.weight = word_pair.second.title_cnt * title_wight + 
                              word_pair.second.content_cnt * content_weight;

                //将倒排元素插入到倒排拉链中
                inverted_index[word_pair.first].push_back(std::move(item));
            }
        }
    };
    Index* Index::instance = nullptr;
    std::mutex Index::mtx;
}