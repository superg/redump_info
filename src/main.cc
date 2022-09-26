#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <list>
#include "common.hh"
#include "dat.hh"
#include "info.hh"
#include "options.hh"
#include "strings.hh"
#include "submission.hh"



using namespace std;
using namespace redump_info;



void recursive_process(void (*callback)(const Options &, const std::filesystem::path &, void *), void *callback_data, const Options &options, const std::string &extension)
{
    for(auto const &p : options.positional)
    {
        if(!filesystem::exists(p))
            throw_line("path doesn't exist (" + p + ")");

        // one file
        if(filesystem::is_regular_file(p))
        {
            callback(options, p, callback_data);
        }
        // recurse directory
        else if(filesystem::is_directory(p))
        {
            if(options.recursive)
            {
                for(auto const &it : filesystem::recursive_directory_iterator(p))

                {
                    // skip anything other than regular file
                    if(!it.is_regular_file())
                        continue;

                    // silently filter by extension
                    if(!extension.empty() && str_lowercase(it.path().extension().generic_string()) != extension)
                        continue;

                    callback(options, it.path(), callback_data);
                }
            }
            else
            {
                for(auto const &it : filesystem::directory_iterator(p))
                {
                    // skip anything other than regular file
                    if(!it.is_regular_file())
                        continue;

                    // silently filter by extension
                    if(!extension.empty() && str_lowercase(it.path().extension().generic_string()) != extension)
                        continue;

                    callback(options, it.path(), callback_data);
                }
            }
        }
        else
            throw_line("path is not regular file or directory (" + p + ")");
    }
}



int main(int argc, char *argv[])
{
    int exit_code(0);

    try
    {
        Options options(argc, const_cast<const char **>(argv));
        options.PrintVersion(cout);

        // print usage
        if(options.help || options.positional.empty())
        {
            options.PrintUsage(cout);
        }
        else
        {
            if(options.mode == Options::Mode::INFO)
            {
                // if no individual info options specified enable all
                bool enable_all = true;
                for(uint32_t i = 0; i < dim(options.info); ++i)
                {
                    if(options.info[i])
                    {
                        enable_all = false;
                        break;
                    }
                }

                if(enable_all)
                    for(uint32_t i = 0; i < dim(options.info); ++i)
                        options.info[i] = true;

                recursive_process(info, nullptr, options, "." + str_lowercase(options.extension));
            }
            else if(options.mode == Options::Mode::SUBMISSION)
            {
                std::unique_ptr<DAT> dat;

                if(filesystem::exists(options.dat_path))
                    dat = make_unique<DAT>(options.dat_path);

                recursive_process(submission, dat.get(), options, ".cue");
            }
            else
            {
                throw_line("mode not implemented (" + options.ModeString() + ")");
            }
        }
    }
    catch(const exception &e)
    {
        cerr << "error: " << e.what() << endl;
        exit_code = 1;
    }
    catch(...)
    {
        cerr << "error: unhandled exception" << endl;
        exit_code = 2;
    }

    ;    return exit_code;
}
