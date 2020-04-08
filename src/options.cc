#include <stdexcept>
#include "common.hh"
#include "options.hh"



namespace redump_info
{

const std::unordered_map<std::string, Options::Mode> Options::_MODES =
{
    {"info", Mode::INFO},
    {"submission", Mode::SUBMISSION}
};


Options::Options()
    : basename("redump_info")
    , mode(Mode::INFO)
    , help(false)
    , verbose(false)
    , extension("bin")
{
    for(uint32_t i = 0; i < dim(info); ++i)
        info[i] = false;
}


Options::Options(int argc, const char *argv[])
    : Options()
{
    // basename
    //FIXME: regex more elegant?
    std::string argv0(argv[0]);
    auto found = argv0.find_last_of("/\\");
    if(found != std::string::npos)
        basename = argv0.substr(found + 1);
    found = basename.find(".");
    if(found != std::string::npos)
        basename = basename.substr(0, found);

    std::string *o_value = nullptr;
    for(int i = 1; i < argc; ++i)
    {
        auto o(argv[i]);
        switch(o[0])
        {
        // option
        case '-':
        {
            if(o_value == nullptr)
            {
                std::string key(o);
                if(key == "--help" || key == "-h")
                    help = true;
                else if(key == "--verbose" || key == "-V")
                    verbose = true;
                else if(key == "--extension" || key == "-e")
                    o_value = &extension;
                // info
                else if(key == "--start-msf")
                    start_msf = true;
                else if(key == "--sector-size")
                    sector_size = true;
                else if(key == "--edc")
                    edc = true;
                else if(key == "--serial")
                    serial = true;
                else if(key == "--launcher")
                    launcher = true;
                else if(key == "--system-area")
                    system_area = true;

                // submission
                else if(key == "--dat-file")
                    o_value = &dat_path;
                // unknown option
                else
                {
                    throw_line("unknown option [" + key + "]");
                }
            }
            else
                throw_line("parse error, expected option value [" + argv[i - 1] + "]");
        }
        break;

        // positional
        default:
            if(o_value != nullptr)
            {
                *o_value = o;
                o_value = nullptr;
            }
            else
                positional.emplace_back(o);
        }
    }

    // parse positional mode
    if(positional.size() > 1)
    {
        auto it = _MODES.find(positional.front());
        if(it != _MODES.end())
        {
            mode = it->second;
            positional.pop_front();
        }
    }
}


std::string Options::ModeString()
{
    std::string mode_string;

    for(auto const &m : _MODES)
        if(m.second == mode)
            return m.first;

    throw_line("mode not found");
}


void Options::PrintVersion(std::ostream &os)
{
    os << basename << " v" << RI_VERSION_MAJOR << "." << RI_VERSION_MINOR << " (" << XSTR(RI_TIMESTAMP) << ")" << std::endl;
    os << std::endl;
}


void Options::PrintUsage(std::ostream &os)
{
    os << "usage: " << basename << " [options] [mode] <path> [path1 ...]" << std::endl;
    os << std::endl;

    os << "modes: " << std::endl;
    os << "\tinfo\tdefault, outputs basic image information" << std::endl;
    os << std::endl;

    os << "options: " << std::endl;
    os << "\t--help,-h\tprint help message" << std::endl;
    os << "\t--verbose,-V\tverbose output" << std::endl;
    os << "\t--extension,-e\tdefault CD image extension" << std::endl;
}

}
