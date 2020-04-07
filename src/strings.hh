#pragma once



#include <string>
#include <vector>



namespace redump_info
{

std::string str_lowercase(const std::string &str);
std::string str_uppercase(const std::string &str);
std::vector<std::string> tokenize(const std::string &str, const char *delimiters);
std::vector<std::string> tokenize_quoted(const std::string &str, const char *delimiters = " ", const char *quotes = "\"\"");
bool string_ends_with(const std::string &value, const std::string &ending);
void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);

}
