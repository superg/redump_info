#include <fstream>
#include "common.hh"
#include "strings.hh"



namespace redump_info
{

std::list<std::string> cue_extract_files(const std::filesystem::path &cue_path)
{
    std::list<std::string> files;

    std::ifstream ifs(cue_path);
    if(ifs.fail())
        throw std::runtime_error("unable to open a file [" + cue_path.generic_string() + "]");

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
