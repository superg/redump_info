#include <fstream>
#include "common.hh"
#include "strings.hh"



namespace redump_info
{

void throw_file_line(const std::string &message, const char *file_str, int line)
{
    // erase absolute path
    std::string file(file_str);
    auto p = file.find("redump_info");
    if(p != std::string::npos)
        file.erase(0, p);

    throw std::runtime_error(message + " [" + file + ":" + std::to_string(line) + "]");
}


std::list<std::string> cue_extract_files(const std::filesystem::path &cue_path)
{
    std::list<std::string> files;

    std::ifstream ifs(cue_path);
    if(ifs.fail())
        throw_line("unable to open a file (" + cue_path.generic_string() + ")");

    std::string line;
    while(std::getline(ifs, line))
    {
        auto tokens(tokenize_quoted(line));
        if(tokens.size() == 3 && tokens[0] == "FILE")
            files.push_back(tokens[1]);
    }

    return files;
}

}
