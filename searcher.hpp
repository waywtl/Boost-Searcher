#pragma once

#include "index.hpp"
#include "util.hpp"
#include "log.hpp"
#include <algorithm>
#include <jsoncpp/json/json.h>

namespace ns_searcher
{
    class Searcher
    {
    private:
        ns_index::Index* index;
    public:
        Searcher(){}
        ~Searcher(){}
    public:
        void InitSearcher(const std::string& input)
        {
            //1.获取或者创建index对象
            index = ns_index::Index::GetInstance();
            //std::cout << "获取index单例成功..." << std::endl;//for debug
            LOG(NORMAL, "获取index单例成功...");
            //2.根据index对象建立索引
            index->BuildIndex(input);
            // std::cout << "建立索引成功..." << std::endl;//for debug
            LOG(NORMAL, "建立正排、倒排索引成功...");
        }

        struct InvertedElemPrint
        {
            uint64_t id;
            int weight;
            std::vector<std::string> words;

            InvertedElemPrint()
                :id(0),weight(0)
            {}
        };

        //query: 搜索关键字
        //json_string: 返回给用户浏览器的结果
        void Search(const std::string& query, std::string* json_string)
        {
            //1.分词：对用户传来的query语句进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::WordSegmentation(query, &words);
            //2.触发：根据分完的各个词，进行index查找
            //ns_index::InvertedList inverted_list_all;
            
            std::unordered_map<uint64_t, InvertedElemPrint> tokens_map;
            std::vector<InvertedElemPrint> inverted_list_all;
            for(std::string word : words)
            {
                boost::to_lower(word);

                //获取分词的倒排拉链
                ns_index::InvertedList* inverted_list = index->GetInvertedList(word);
                if(nullptr == inverted_list)
                {
                    continue;
                }
                //将倒排拉链中的倒排元素汇总的放入到一个数组中
                //需要将doc_id相同的elem合并
                //inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());
                //去重
                for(const auto& elem : *inverted_list)
                {
                    auto& item = tokens_map[elem.doc_id];
                    item.id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }
            }
            for(const auto& item : tokens_map)
            {
                inverted_list_all.push_back(std::move(item.second));
            }

            //3.合并排序：汇总查找结果，按照相关性(weight)降序排序
            // std::sort(inverted_list_all.begin(), inverted_list_all.end(), 
            //         [](const ns_index::InvertedElem& e1, const ns_index::InvertedElem& e2)
            //         {return e1.weight > e2.weight;});

            std::sort(inverted_list_all.begin(), inverted_list_all.end(), 
                    [](const InvertedElemPrint& e1, const InvertedElemPrint& e2)
                    {return e1.weight > e2.weight;});

            //4.构建：根据汇总并排序后的数据，构建json串
            Json::Value root;

            if(inverted_list_all.empty())
            {
                Json::Value _empty;
                _empty["title"] = "back to root";
                _empty["desc"] = "No valid content found!";
                _empty["url"] = "https://www.boost.org/";
                root.append(_empty);
            }
            
            // Json::Value my_ad;
            // my_ad["title"] = "waywtl blog";
            // my_ad["desc"] = "a nice blog, deserve to subscribe!";
            // my_ad["url"] = "https://blog.csdn.net/weixin_60954394?type=blog";
            // root.append(my_ad);

            for(const InvertedElemPrint& elem : inverted_list_all)
            {
                //通过每个倒排元素的doc_id获取正排索引
                ns_index::DocInfo* doc = index->GetForwardIndex(elem.id);
                if(nullptr == doc)
                {
                    continue;
                }

                //构建json串
                Json::Value item;
                item["title"] = doc->title;
                item["desc"] = GetDesc(doc->content, elem.words[0]);//需要显示的是摘要，不是内容
                item["url"] = doc->url;

                root.append(item);
            }

            Json::FastWriter writer;
            *json_string = writer.write(root);
        }

        std::string GetDesc(const std::string& content, const std::string& word)
        {
            //找到word在content中首次出现的位置，分别向前与向后截取一定长度作为desc
            //1.word首次出现
            //std::size_t pos = content.find(word);//当文档中对应的词出现大写字母会匹配不上
            auto iter = std::search(content.begin(), content.end(), word.begin(), word.end(), 
                                    [](char c1, char c2){return (std::tolower(c1) == std::tolower(c2));});
            if(iter == content.end())
            {
                return "None1";
            }
            std::size_t pos = std::distance(content.begin(), iter);
            
            //2.自定义需要截取的start与end
            const size_t prev_step = 50;
            const size_t next_step = 100;

            std::size_t start = 0;
            std::size_t end = content.size()-1; 

            if(pos >= start + prev_step)
                start = pos - prev_step;
            if(pos + next_step <= end)
                end = pos + next_step;
            
            if(start >= end)
                return "None2";

            //3.截取子串
            std::string desc = content.substr(start, end-start);
            desc += "...";
            return desc;
        }
    };
}