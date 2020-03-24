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
        INFO
    };

    static const std::unordered_map<std::string, Mode> MODES;

    std::string basename;
    Mode mode;
    bool help;
    bool verbose;

    std::list<std::string> positional;

    union
    {
        struct
        {
            bool sector_size;
            bool edc;
            bool serial;
            bool system_area;
        };
        bool info[4];
    };

    Options();
    Options(int argc, const char *argv[]);

    void PrintVersion(std::ostream &os);
    void PrintUsage(std::ostream &os);
};

}
