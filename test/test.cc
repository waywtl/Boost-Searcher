#include "searcher.hpp"

const std::string input = "data/raw_html/raw.txt";
int main()
{
    ns_searcher::Searcher* searcher = new ns_searcher::Searcher();
    searcher->InitSearcher(input);
    std::string query;
    std::string json_string;
    while(true)
    {
        std::cout << "Please Enter: ";
        std::getline(std::cin, query);
        searcher->Search(query, &json_string);
        std::cout << json_string << std::endl;
    }

    return 0;
}