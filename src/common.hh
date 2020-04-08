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
#define throw_line(arg__) throw std::runtime_error(std::string() + arg__ + " {" + __FILE__ + ":" + std::to_string(__LINE__) + "}")



namespace redump_info
{

// count of static array elements
template <typename T, int N>
constexpr size_t dim(T(&)[N])
{
    return N;
}

std::list<std::string> cue_extract_files(const std::filesystem::path &cue_path);

}
