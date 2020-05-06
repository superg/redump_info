#pragma once



#include <filesystem>
#include "options.hh"



namespace redump_info
{

enum class DiscSystem
{
    PC,
    PSX
};


void submission(const Options &o, const std::filesystem::path &f, void *data);
void submission_test(const Options &o, const std::filesystem::path &f, void *data);

}
