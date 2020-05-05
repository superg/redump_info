#pragma once



#include <cstddef>
#include <filesystem>
#include <list>
#include <stdexcept>
#include <string>



// stringify
#define XSTR(arg__) STR(arg__)
#define STR(arg__) #arg__

// meaningful throw
#define throw_line(arg__) throw_file_line(std::string() + arg__, __FILE__, __LINE__)



namespace redump_info
{

// count of static array elements
template <typename T, int N>
constexpr size_t dim(T(&)[N])
{
    return N;
}

void throw_file_line(const std::string &message, const char *file, int line);

std::list<std::string> cue_extract_files(const std::filesystem::path &cue_path);

}
