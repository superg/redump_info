#pragma once



#include <ostream>
#include <string>
#include "image_browser.hh"



namespace redump_info::psx
{

std::string extract_exe_path(ImageBrowser &browser);
std::pair<std::string, std::string> extract_serial_pair(ImageBrowser &browser);
std::string extract_serial(ImageBrowser &browser);
std::string detect_region(const std::string &prefix);
std::string extract_region(ImageBrowser &browser);
std::vector<std::string> detect_anti_modchip_string(ImageBrowser &browser);
void detect_libcrypt(std::ostream &os, const std::filesystem::path &sub_file, const std::filesystem::path &sbi_file);
void detect_libcrypt_redumper(std::ostream &os, const std::filesystem::path &sub_file, const std::filesystem::path &sbi_file);

}
