#include <string>
#include <regex>
#include <iostream>


class HTTPParser {
public:
    HTTPParser(std::string s) {
        //std::regex path_regex("GET (/*) .*");
        std::regex path_regex("GET ([\\/\\w\\.]*)[\\? ]");
        std::smatch match;
        if (regex_search(s, match, path_regex))
        {
            std::string path = match[1];
            std::cout << "\"" << path << "\"" << std::endl;

        }
    }
};
