#pragma once



#include <string>
#include <vector>



namespace redump_info
{

std::string str_lowercase(const std::string &str);
std::string str_uppercase(const std::string &str);
std::vector<std::string> tokenize(const std::string &str, const char *delimiters);
std::vector<std::string> tokenize_quoted(const std::string &str, const char *delimiters = " ", const char *quotes = "\"\"");
void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
void replace_all_occurences(std::string &str, const std::string &from, const std::string &to);

}
