#include <cstddef>
#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include "common.hh"
#include "image_browser.hh"
#include "psx.hh"
#include "strings.hh"
#include "info.hh"



using namespace std;



namespace redump_info
{

string start_msf(const std::filesystem::path &f)
{
	ifstream ifs(f, ifstream::binary);
	if(ifs.fail())
		throw_line("unable to open file [" + std::strerror(errno) + "]");

	ifs.seekg(offsetof(cdrom::Sector, header.address));
	if(ifs.fail())
		throw_line("seek failure");
	cdrom::Sector::Header::Address address;
	ifs.read((char *)&address, sizeof(address));
	if(ifs.fail())
		throw_line(std::string("read failure [") + std::strerror(errno) + "]");

	std::stringstream ss;
	ss << hex << setfill('0') << setw(2) << (unsigned)address.minute << ':' << setw(2) << (unsigned)address.second << '.' << dec << setw(3) << (unsigned)address.frame;

	return ss.str();
}


bool mode2form2_edc_fast(const std::filesystem::path &f)
{
	bool edc = false;

	ifstream ifs(f, ifstream::binary);
	if(ifs.fail())
		throw_line("unable to open file [" + std::strerror(errno) + "]");

	// easier to iterate this way
	uint64_t sectors_count = filesystem::file_size(f);
	for(uint64_t i = 0; i < sectors_count; ++i)
	{
		uint8_t mode;
		ifs.seekg(i * sizeof(cdrom::Sector) + offsetof(cdrom::Sector, header.mode));
		ifs.read((char *)&mode, sizeof(mode));

		// can we have mixed mode 1 mode 2 sectors in one track?
		if(mode == 1)
			break;

		if(mode == 2)
		{
			uint8_t submode;
			ifs.seekg(i * sizeof(cdrom::Sector) + offsetof(cdrom::Sector, mode2.xa.sub_header.submode));
			ifs.read((char *)&submode, sizeof(submode));
			if(submode & (uint8_t)cdrom::CDXAMode::FORM2)
			{
				uint32_t sector_edc;
				ifs.seekg(i * sizeof(cdrom::Sector) + offsetof(cdrom::Sector, mode2.xa.form2.edc));
				ifs.read((char *)&sector_edc, sizeof(sector_edc));

				edc = sector_edc;
				break;
			}
		}
	}

	return edc;
}


void info(const Options &o, const std::filesystem::path &f)
{
	try
	{
		cout << f.generic_string() << ": " << endl;

		ImageBrowser browser(f);
//		auto pvd = browser.GetPVD();

		if(o.start_msf)
			cout << "\tStart MSF: " << start_msf(f) << endl;

		if(o.sector_size)
			cout << "\tSectors count: " << filesystem::file_size(f) / sizeof(cdrom::Sector) << endl;

		if(o.edc)
			cout << "\tMode2Form2 EDC: " << (mode2form2_edc_fast(f) ? "Yes" : "No") << endl;

		if(o.launcher)
		{
			string launcher = psx::extract_exe_path(browser);
			cout << "\tLauncher: " << (launcher.empty() ? "<unavailable>" : launcher) << endl;
		}

		if(o.serial)
		{
			string serial = psx::extract_serial(browser);
			cout << "\tSerial: " << (serial.empty() ? "<unavailable>" : serial) << endl;
		}
	}
	catch(const std::exception &e)
	{
		if(o.verbose)
			cout << f.generic_string() << ": skipped [" << e.what() << "]" << endl;
	}
}

}
