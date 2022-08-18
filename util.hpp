#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"
#include "log.hpp"

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            std::ifstream in(file_path);
            if (!in.is_open())
            {
                std::cerr << "open file" << file_path << "failed!" << std::endl;
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                *out += line;
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void CutString(const std::string &target, std::vector<std::string> *out, const std::string sep)
        {
            // boost split
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
        }
    };

    const char *const DICT_PATH = "./dict/jieba.dict.utf8";
    const char *const HMM_PATH = "./dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char *const IDF_PATH = "./dict/idf.utf8";
    const char *const STOP_WORD_PATH = "./dict/stop_words.utf8";

    class JiebaUtil
    {
    private:
        cppjieba::Jieba jieba;
        std::unordered_set<std::string> stop_words;
    private:
        static JiebaUtil* instance;
        static std::mutex mtx;

        static JiebaUtil* GetInstance()
        {
            if(instance == nullptr)
            {
                mtx.lock();
                if(instance == nullptr)
                {
                    instance = new JiebaUtil();
                    instance->InitJiebaUtil();
                }
                mtx.unlock();
            }
            return instance;
        } 

        JiebaUtil()
            :jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH)
        {}

        JiebaUtil(const JiebaUtil&) = delete;
        JiebaUtil operator=(const JiebaUtil&) = delete;
        
        void InitJiebaUtil()
        {
            std::ifstream in(STOP_WORD_PATH);
            if(!in.is_open())
            {
                LOG(FATAL, "open stop words file failed!");
                return;
            }

            std::string line;
            while(std::getline(in, line))
            {
                stop_words.insert(line);
            }
            in.close();
        }

        void WordSegmentationHelper(const std::string& src, std::vector<std::string>* out)
        {
            jieba.CutForSearch(src, *out);
            for(auto iter = out->begin(); iter != out->end();)
            {
                if(stop_words.find(*iter) != stop_words.end())
                {
                    iter = out->erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    public:
        static void WordSegmentation(const std::string& src, std::vector<std::string>* out)
        {
            JiebaUtil::GetInstance()->WordSegmentationHelper(src, out);
            //jieba.CutForSearch(src, *out);
        }
    };
    JiebaUtil* JiebaUtil::instance = nullptr;
    std::mutex JiebaUtil::mtx;
    
    // class JiebaUtil
    // {
    // private:
    //     static cppjieba::Jieba jieba;
    // public:
    //     static void WordSegmentation(const std::string& src, std::vector<std::string>* out)
    //     {
    //         jieba.CutForSearch(src, *out);
    //     }
    // };

    // cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
}