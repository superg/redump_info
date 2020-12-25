#include <cstddef>
#include <ctime>
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
		throw_line("unable to open file (" + std::strerror(errno) + ")");

	ifs.seekg(offsetof(cdrom::Sector, header.address));
	if(ifs.fail())
		throw_line("seek failure");
	cdrom::Sector::Header::Address address;
	ifs.read((char *)&address, sizeof(address));
	if(ifs.fail())
		throw_line(std::string("read failure (") + std::strerror(errno) + ")");

	std::stringstream ss;
	ss << hex << setfill('0') << setw(2) << (unsigned)address.minute << ':' << setw(2) << (unsigned)address.second << '.' << dec << setw(3) << (unsigned)address.frame;

	return ss.str();
}


bool mode2form2_edc_fast(const std::filesystem::path &f)
{
	bool edc = false;

	ifstream ifs(f, ifstream::binary);
	if(ifs.fail())
		throw_line("unable to open file (" + std::strerror(errno) + ")");

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


void info(const Options &o, const std::filesystem::path &f, void *)
{
	try
	{
		ImageBrowser browser(f);

		cout << f.generic_string();
		if(o.batch)
			cout << ",";
		else
			cout << ": " << endl;

		if(o.start_msf)
		{
			if(!o.batch)
				cout << "\tStart MSF: ";
			cout << start_msf(f) << endl;
		}

		if(o.sector_size)
		{
			if(!o.batch)
				cout << "\tSectors count: ";
			cout << filesystem::file_size(f) / sizeof(cdrom::Sector) << endl;
		}

		if(o.edc)
		{
			if(!o.batch)
				cout << "\tMode2Form2 EDC: ";
			cout << (mode2form2_edc_fast(f) ? "Yes" : "No") << endl;
		}

		if(o.pvd_time)
		{
			auto &pvd = browser.GetPVD();

			time_t time_newest = iso9660::convert_time(pvd.primary.volume_creation_date_time);
			/*
			// DEBUG
			{
			char buffer[32];
			strftime(buffer, 32, "%Y-%m-%d", localtime(&time_newest));
			cout << "[PVD]: " << buffer << endl;
			}
			*/
			browser.Iterate([&](const std::string &path, std::shared_ptr<ImageBrowser::Entry> d)
							{
								bool exit = false;

								time_t file_time = d->DateTime();
								/*
								// DEBUG
								{
								auto fp((path.empty() ? "" : path + "/") + d->Name());

								char buffer[32];
								strftime(buffer, 32, "%Y-%m-%d", localtime(&file_time));
								cout << fp << ": " << buffer << endl;
								}
								*/
								if(file_time > time_newest)
									time_newest = file_time;

								return exit;
							});

			{
				char buffer[32];
				//				strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", localtime(&time_newest));
				strftime(buffer, 32, "%Y-%m-%d", localtime(&time_newest));
				cout << buffer << endl;
			}

			//			cout << "";
		}

		if(o.file_offsets)
		{
			browser.Iterate([&](const std::string &path, std::shared_ptr<ImageBrowser::Entry> d)
							{
								bool exit = false;

								uint32_t sector_offset = d->_directory_record.offset.lsb;
								uint32_t sector_size = d->SectorSize();

								std::cout << d->Name()
									<< ", sector offset: " << sector_offset
									<< ", sector size: " << sector_size
									<< " [0x" << std::hex << std::setfill('0') << sector_offset * sizeof(cdrom::Sector) << std::setfill(' ') << std::dec
									<< ", " << sector_size * sizeof(cdrom::Sector) << "]"
									<< std::endl;
								
								return exit;
							});
		}

		if(o.launcher)
		{
			string launcher = psx::extract_exe_path(browser);

			if(!o.batch)
				cout << "\tLauncher: ";
			cout << (launcher.empty() ? "<unavailable>" : launcher) << endl;
		}

		if(o.serial)
		{
			string serial = psx::extract_serial(browser);

			if(!o.batch)
				cout << "\tSerial: ";
			cout << (serial.empty() ? "<unavailable>" : serial) << endl;
		}

		if(o.system_area)
		{
			//TODO
		}

		if(o.antimod)
		{
			auto entries = psx::detect_anti_modchip_string(browser);
			if(!entries.empty())
			{
				if(!o.batch)
					cout << "\tAnti-Modchip: " << endl;

				for(auto const &e : entries)
					cout << "\t\t" << e << endl;
			}
		}
	}
	catch(const std::exception &e)
	{
		if(o.verbose)
			cout << f.generic_string() << ": skipped {" << e.what() << "}" << endl;
	}
}

}
