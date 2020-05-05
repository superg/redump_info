#pragma once



#include <list>
#include <ostream>
#include <string>
#include <unordered_map>



namespace redump_info
{

struct Options
{
    enum class Mode
    {
        INFO,
        SUBMISSION
    };

    static const std::unordered_map<std::string, Mode> _MODES;

    std::string basename;
    Mode mode;
    bool help;
    bool verbose;
    bool batch;
    bool overwrite;
    std::string extension;

    std::list<std::string> positional;

    // info
    union
    {
        struct
        {
            bool start_msf;
            bool sector_size;
            bool edc;
            bool launcher;
            bool serial;
            bool system_area;
        };
        bool info[6];
    };

    // submission
    std::string dat_path;

    Options();
    Options(int argc, const char *argv[]);

    std::string ModeString();

    void PrintVersion(std::ostream &os);
    void PrintUsage(std::ostream &os);
};

}
