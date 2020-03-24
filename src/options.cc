#include <stdexcept>
#include "options.hh"



namespace redump_info
{

const std::unordered_map<std::string, Options::Mode> Options::MODES =
{
    {"info", Mode::INFO}
};


Options::Options()
    : basename("redump_info")
    , mode(Mode::INFO)
    , help(false)
    , verbose(false)
{
    for(uint32_t i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
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

    for(int i = 1; i < argc; ++i)
    {
        auto o(argv[i]);
        switch(o[0])
        {
            // option
        case '-':
        {
            std::string key(o);
            if(key == "--help" || key == "-h")
                help = true;
            else if(key == "--verbose" || key == "-V")
                verbose = true;
            // unknown option
            else
            {
                throw std::runtime_error("unknown option " + key);
            }
        }
        break;

        // positional
        default:
            positional.emplace_back(o);
        }
    }

    // parse positional mode
    if(positional.size() > 1)
    {
        auto it = MODES.find(positional.front());
        if(it != MODES.end())
        {
            mode = it->second;
            positional.pop_front();
        }
    }
}


#define xstr(arg_) str(arg_)
#define str(arg_) #arg_
void Options::PrintVersion(std::ostream &os)
{
    os << basename << " v" << RI_VERSION_MAJOR << "." << RI_VERSION_MINOR << " (" << xstr(RI_TIMESTAMP) << ")" << std::endl;
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
}

}
