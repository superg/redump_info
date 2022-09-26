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
    // common
    , help(false)
    , verbose(false)
    , recursive(false)
    , extension("bin")
    // info
    , batch(false)
    // submission
    , overwrite(false)
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
                // common
                std::string key(o);
                if(key == "--help" || key == "-h")
                    help = true;
                else if(key == "--verbose" || key == "-V")
                    verbose = true;
                else if(key == "--recursive" || key == "-R")
                    recursive = true;
                else if(key == "--extension" || key == "-e")
                    o_value = &extension;

                // info
                else if(key == "--start-msf")
                    start_msf = true;
                else if(key == "--sector-size")
                    sector_size = true;
                else if(key == "--edc")
                    edc = true;
                else if(key == "--pvd-time")
                    pvd_time = true;
                else if(key == "--file-offsets")
                    file_offsets = true;

                // info PSX
                else if(key == "--launcher")
                    launcher = true;
                else if(key == "--serial")
                    serial = true;
                else if(key == "--system-area")
                    system_area = true;
                else if(key == "--antimod")
                    antimod = true;

                else if(key == "--batch")
                    batch = true;

                // submission
                else if(key == "--dat-file")
                    o_value = &dat_path;
                else if(key == "--overwrite")
                    overwrite = true;
                else if(key == "--mastering-code")
                {
                    mastering_code = std::make_unique<std::string>();
                    o_value = mastering_code.get();
                }
                else if(key == "--mastering-sid")
                {
                    mastering_sid = std::make_unique<std::string>();
                    o_value = mastering_sid.get();
                }
                else if(key == "--data-mould-sid")
                {
                    data_mould_sid = std::make_unique<std::string>();
                    o_value = data_mould_sid.get();
                }
                else if(key == "--label-mould-sid")
                {
                    label_mould_sid = std::make_unique<std::string>();
                    o_value = label_mould_sid.get();
                }
                else if(key == "--additional-mould")
                {
                    additional_mould = std::make_unique<std::string>();
                    o_value = additional_mould.get();
                }
                else if(key == "--toolstamp")
                {
                    toolstamp = std::make_unique<std::string>();
                    o_value = toolstamp.get();
                }
                else if(key == "--contents")
                {
                    contents = std::make_unique<std::string>();
                    o_value = contents.get();
                }
                else if(key == "--version")
                {
                    version = std::make_unique<std::string>();
                    o_value = version.get();
                }
                else if(key == "--edition")
                {
                    edition = std::make_unique<std::string>();
                    o_value = edition.get();
                }
                // unknown option
                else
                {
                    throw_line("unknown option (" + key + ")");
                }
            }
            else
                throw_line("parse error, expected option value (" + argv[i - 1] + ")");
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
        {
            mode_string = m.first;
            break;
        }

    return mode_string;
}


void Options::PrintVersion(std::ostream &os)
{
    os << basename << " v" << XSTR(RI_VERSION_MAJOR) << "." << XSTR(RI_VERSION_MINOR) << " (" << XSTR(RI_TIMESTAMP) << ")" << std::endl;
    os << std::endl;
}


void Options::PrintUsage(std::ostream &os)
{
    os << "usage: " << basename << " [options] [mode] <path> [path1 ...]" << std::endl;
    os << std::endl;

    os << "modes: " << std::endl;
    os << "\tinfo\t\tdefault mode, outputs basic data track information" << std::endl;
    os << "\tsubmission\tgenerates !submissionInfo_*.txt for further submission to redump.org" << std::endl;
    os << std::endl;

    os << "path: " << std::endl;
    os << "\tcan be either a file path or a directory path which will be processed recursively" << std::endl;
    os << std::endl;

    os << "general options: " << std::endl;
    os << "\t--help,-h\tprint help message" << std::endl;
    os << "\t--verbose,-V\tverbose output" << std::endl;
    os << "\t--recursive,-R\trecursively process subdirectories" << std::endl;
    os << "\t--extension,-e\tdefault CD track extension [bin]" << std::endl;
    os << std::endl;

    os << "info options: " << std::endl;
    os << "\t--start-msf\tprint start MSF address" << std::endl;
    os << "\t--sector-size\tprint sector size" << std::endl;
    os << "\t--edc\t\tprint EDC information" << std::endl;
    os << "\t--pvd-time\t\tprint PVD creation date/time" << std::endl;
    os << "\t--file-offsets\t\tprint ISO9660 file offsets" << std::endl;

    // PSX
    os << "\t--launcher\tprint startup executable path (PSX)" << std::endl;
    os << "\t--serial\tprint print disc serial (PSX)" << std::endl;
    os << "\t--system-area\tprint system area information (PSX)" << std::endl;
    os << "\t--antimod\tprint antimod string locations (PSX)" << std::endl;

    os << "\t--batch\t\tbatch mode, supress formatted output" << std::endl;
    os << std::endl;

    os << "submission options: " << std::endl;
    os << "\t--dat-file\t\t\tpath to redump DAT file" << std::endl;
    os << "\t--overwrite\t\t\toverwrite generated !submissionInfo_*.txt" << std::endl;
    os << "\t--mastering-code <value>\tfill \"Mastering Code\" field with value" << std::endl;
    os << "\t--mastering-sid <value>\t\tfill \"Mastering SID Code\" field with value" << std::endl;
    os << "\t--data-mould-sid <value>\tfill \"Data-Side Mould SID Code\" field with value" << std::endl;
    os << "\t--label-mould-sid <value>\tfill \"Label-Side Mould SID Code\" field with value" << std::endl;
    os << "\t--additional-mould <value>\tfill \"Additional Mould\" field with value" << std::endl;
    os << "\t--toolstamp <value>\t\tfill \"Toolstamp or Mastering Code\" field with value" << std::endl;
    os << "\t--contents <value>\t\tfill \"Contents\" field with value" << std::endl;
    os << "\t--version <value>\t\tfill \"Version\" field with value" << std::endl;
    os << "\t--edition <value>\t\tfill \"Edition/Release\" field with value" << std::endl;
}

}
