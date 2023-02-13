#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

void ltrim(std::string& s);

void rtrim(std::string& s);

void trim(std::string& s);

std::vector<std::string> split_line(const std::string& line, char delim);

#endif
