#pragma once



#include <filesystem>
#include "options.hh"



namespace redump_info
{

void info(const Options &o, const std::filesystem::path &f);

}
