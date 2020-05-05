#pragma once



#include <filesystem>
#include "options.hh"



namespace redump_info
{

void submission(const Options &o, const std::filesystem::path &f, void *data);
void submission_test(const Options &o, const std::filesystem::path &f, void *data);

}
