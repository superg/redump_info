#include <algorithm>
#include <set>
#include "strings.hh"



namespace redump_info
{

std::string str_lowercase(const std::string &str)
{
    std::string str_lc;
    std::transform(str.begin(), str.end(), std::back_inserter(str_lc), [](unsigned char c) { return std::tolower(c); });

    return str_lc;
}


std::string str_uppercase(const std::string &str)
{
    std::string str_uc;
    std::transform(str.begin(), str.end(), std::back_inserter(str_uc), [](unsigned char c) { return std::toupper(c); });

    return str_uc;
}


std::vector<std::string> tokenize(const std::string &str, const char *delimiters)
{
    std::vector<std::string> tokens;

    std::set<char> delimiter;
    for(auto d = delimiters; *d != '\0'; ++d)
        delimiter.insert(*d);

    bool in = false;
    std::string::const_iterator s;
    for(auto it = str.begin(); it < str.end(); ++it)
    {
        if(in)
        {
            if(delimiter.find(*it) != delimiter.end())
            {
                tokens.emplace_back(s, it);
                in = false;
            }
        }
        else
        {
            if(delimiter.find(*it) == delimiter.end())
            {
                s = it;
                in = true;
            }
        }
    }

    // remaining entry
    if(in)
        tokens.emplace_back(s, str.end());

    return tokens;
}


std::vector<std::string> tokenize_quoted(const std::string &str, const char *delimiters, const char *quotes)
{
    std::vector<std::string> tokens;

    std::set<char> delimiter;
    for(auto d = delimiters; *d != '\0'; ++d)
        delimiter.insert(*d);

    bool in = false;
    std::string::const_iterator s;
    for(auto it = str.begin(); it < str.end(); ++it)
    {
        if(in)
        {
            // quoted
            if(*s == quotes[0])
            {
                if(*it == quotes[1])
                {
                    ++s;
                    tokens.emplace_back(s, it);
                    in = false;
                }
            }
            // unquoted
            else
            {
                if(delimiter.find(*it) != delimiter.end())
                {
                    tokens.emplace_back(s, it);
                    in = false;
                }
            }
        }
        else
        {
            if(delimiter.find(*it) == delimiter.end())
            {
                s = it;
                in = true;
            }
        }
    }

    // remaining entry
    if(in)
        tokens.emplace_back(s, str.end());

    return tokens;
}


void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
}


void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(), s.end());
}


void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}


void replace_all_occurences(std::string &str, const std::string &from, const std::string &to)
{
    for(size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.length())
        str.replace(pos, from.length(), to);
}

}
