#pragma once



#include <list>
#include <memory>
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

    Mode mode;

    std::string basename;
    std::list<std::string> positional;

    // common
    bool help;
    bool verbose;
    std::string extension;

    // info
    union
    {
        struct
        {
            bool start_msf;
            bool sector_size;
            bool edc;
            bool pvd_time;
            bool file_offsets;

            // PSX
            bool launcher;
            bool serial;
            bool system_area;
            bool antimod;
        };
        bool info[9];
    };
    bool batch;

    // submission
    std::string dat_path;
    bool overwrite;
    // lazy way to distinguish between no key and key with an empty value
    std::unique_ptr<std::string> mastering_code;
    std::unique_ptr<std::string> mastering_sid;
    std::unique_ptr<std::string> data_mould_sid;
    std::unique_ptr<std::string> label_mould_sid;
    std::unique_ptr<std::string> additional_mould;
    std::unique_ptr<std::string> toolstamp;
    std::unique_ptr<std::string> contents;
    std::unique_ptr<std::string> version;
    std::unique_ptr<std::string> edition;

    Options();
    Options(int argc, const char *argv[]);

    std::string ModeString();

    void PrintVersion(std::ostream &os);
    void PrintUsage(std::ostream &os);
};

}
