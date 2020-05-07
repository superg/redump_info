#pragma once



#include <filesystem>
#include "options.hh"



namespace redump_info
{

enum class DiscSystem
{
    AUDIO,
    DATA,
    PC,
    PSX
};


void submission(const Options &o, const std::filesystem::path &f, void *data);
void submission_test(const Options &o, const std::filesystem::path &f, void *data);

}
