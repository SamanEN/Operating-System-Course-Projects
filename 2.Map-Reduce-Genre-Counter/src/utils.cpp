#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
}

void trim(std::string& s) {
    rtrim(s);
    ltrim(s);
}

std::vector<std::string> split_line(const std::string& line, char delim) {
    std::vector<std::string> result;
    std::istringstream line_stream(line);
    while (!line_stream.eof()) {
        std::string temp;
        std::getline(line_stream, temp, delim);
        trim(temp);
        result.push_back(temp);
    }
    return result;
}
