﻿#include <iomanip>
#include <regex>
#include <set>
#include <sstream>
#include <vector>
#include "common.hh"
#include "crc16.hh"
#include "endian.hh"
#include "strings.hh"
#include "psx.hh"



namespace redump_info::psx
{

std::string extract_exe_path(ImageBrowser &browser)
{
	std::string exe_path;

	auto system_cnf = browser.RootDirectory()->SubEntry("SYSTEM.CNF");
	if(system_cnf)
	{
		auto data = system_cnf->Read();
		std::string data_str(data.begin(), data.end());
		std::stringstream ss(data_str);

		std::string line;
		while(std::getline(ss, line))
		{
			// examples:
            // BOOT = cdrom:\\SCUS_945.03;1\r"   // 1Xtreme (USA)
            // BOOT=cdrom:\\SCUS_944.23;1"       // Ape Escape (USA)
            // BOOT=cdrom:\\SLPS_004.35\r"       // Megatudo 2096 (Japan)
            // BOOT = cdrom:\SLPM803.96;1"       // Chouzetsu Daigirin '99-nen Natsu-ban (Japan)
            // BOOT = cdrom:\EXE\PCPX_961.61;1   // Wild Arms - 2nd Ignition (Japan) (Demo)

			std::smatch matches;
			std::regex_match(line, matches, std::regex("^\\s*BOOT.*=\\s*cdrom.?:\\\\*(.*?)(?:;.*\\s*|\\s*$)"));
			if(matches.size() == 2)
			{
				exe_path = str_uppercase(matches.str(1));
				break;
			}
		}
	}
	else
	{
		auto psx_exe = browser.RootDirectory()->SubEntry("PSX.EXE");
		if(psx_exe)
			exe_path = psx_exe->Name();
	}

	return exe_path;
}


std::pair<std::string, std::string> extract_serial_pair(ImageBrowser &browser)
{
    std::pair<std::string, std::string> serial;

    std::string exe_path = extract_exe_path(browser);

    std::smatch matches;
    std::regex_match(exe_path, matches, std::regex("(.*\\\\)*([A-Z]*)(_|-)?([A-Z]?[0-9]+)\\.([0-9]+[A-Z]?)"));
    if(matches.size() == 6)
    {
        serial.first = matches.str(2);
        serial.second = matches.str(4) + matches.str(5);

        // Road Writer (USA)
        if(serial.first.empty() && serial.second == "907127001")
            serial.first = "LSP";
        // GameGenius Ver. 5.0 (Taiwan) (En,Zh) (Unl)
        else if(serial.first == "PAR" && serial.second == "90001")
        {
            serial.first.clear();
            serial.second.clear();
        }
    }

    return serial;
}


std::string extract_serial(ImageBrowser &browser)
{
    auto p = extract_serial_pair(browser);
    
    std::string serial;
    if(!p.first.empty() || !p.second.empty())
        serial = p.first + "-" + p.second;

    return serial;
}


std::string detect_region(const std::string &prefix)
{
    std::string region;

    const std::set<std::string> REGION_J {"ESPM", "PAPX", "PCPX", "PDPX", "SCPM", "SCPS", "SCZS", "SIPS", "SLKA", "SLPM", "SLPS"};
    const std::set<std::string> REGION_U {"LSP", "PEPX", "SCUS", "SLUS", "SLUSP"};
    const std::set<std::string> REGION_E {"PUPX", "SCED", "SCES", "SLED", "SLES"};
    // multi: "DTL", "PBPX"

    if(REGION_J.find(prefix) != REGION_J.end())
        region = "Japan";
    else if(REGION_U.find(prefix) != REGION_U.end())
        region = "USA";
    else if(REGION_E.find(prefix) != REGION_E.end())
        region = "Europe";

    return region;
}


std::string extract_region(ImageBrowser &browser)
{
    return detect_region(extract_serial_pair(browser).first);
}


std::vector<std::string> detect_anti_modchip_string(ImageBrowser &browser)
{
    std::vector<std::string> entries;

	// taken from DIC
	const char ANTIMOD_MESSAGE_EN[] = "     SOFTWARE TERMINATED\nCONSOLE MAY HAVE BEEN MODIFIED\n     CALL 1-888-780-7690";
	// string is encoded with Shift JIS
	const uint8_t ANTIMOD_MESSAGE_JP[] =
	{
        // 強制終了しました。
		0x8b, 0xad, 0x90, 0xa7, 0x8f, 0x49, 0x97, 0xb9, 0x82, 0xb5, 0x82, 0xdc, 0x82, 0xb5, 0x82, 0xbd, 0x81, 0x42, 0x0a,
        // 本体が改造されている
		0x96, 0x7b, 0x91, 0xcc, 0x82, 0xaa, 0x89, 0xfc, 0x91, 0xa2, 0x82, 0xb3, 0x82, 0xea, 0x82, 0xc4, 0x82, 0xa2, 0x82, 0xe9, 0x0a,
        // おそれがあります。
		0x82, 0xa8, 0x82, 0xbb, 0x82, 0xea, 0x82, 0xaa, 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xdc, 0x82, 0xb7, 0x81, 0x42
	};

	browser.Iterate([&](const std::string &path, std::shared_ptr<ImageBrowser::Entry> d)
	{
		bool exit = false;

		auto fp((path.empty() ? "" : path + "/") + d->Name());

		if(!d->IsDummy() && !d->IsInterleaved())
		{
			auto data = d->Read(false, false);

			auto it_en = search(data.begin(), data.end(), std::begin(ANTIMOD_MESSAGE_EN), std::end(ANTIMOD_MESSAGE_EN));
            if(it_en != data.end())
            {
                std::stringstream ss;
                ss << fp << " @ 0x" << std::hex << it_en - data.begin() << ": EN";
                entries.emplace_back(ss.str());
            }
			auto it_jp = search(data.begin(), data.end(), std::begin(ANTIMOD_MESSAGE_JP), std::end(ANTIMOD_MESSAGE_JP));
            if(it_jp != data.end())
            {
                std::stringstream ss;
                ss << fp << " @ 0x" << std::hex << it_jp - data.begin() << ": JP";
                entries.emplace_back(ss.str());
            }
		}

		return exit;
	});

    return entries;
}


void detect_libcrypt(std::ostream &os, const std::filesystem::path &sub_file, const std::filesystem::path &sbi_file)
{
    uint32_t file_size = (uint32_t)std::filesystem::file_size(sub_file);
    if(file_size % (sizeof(cdrom::SubQ) * 8))
        throw_line("subchannel file is incomplete (" + sub_file.generic_string() + ")");

    std::ifstream ifs(sub_file, std::ifstream::binary);
    if(ifs.fail())
        throw_line("unable to open subchannel file (" + sub_file.generic_string() + ")");

    std::ofstream ofs(sbi_file, std::ifstream::binary);
    if(ofs.fail())
        throw_line("unable to create SBI file (" + sbi_file.generic_string() + ")");
    ofs.write("SBI", 4);

    // skip first P
    ifs.seekg(sizeof(cdrom::SubQ), std::ifstream::cur);

    for(uint32_t i = 0, n = file_size / (sizeof(cdrom::SubQ) * 8); i < n; ++i)
    {
        cdrom::SubQ Q;
        ifs.read((char *)&Q, sizeof(Q));

        // skip R, S, T, U, V, W, P
        ifs.seekg(sizeof(cdrom::SubQ) * 7, std::ifstream::cur);

        uint16_t crc_le = crc16_gsm(Q.raw, sizeof(Q.raw));
        if(crc_le != endian_swap(Q.crc))
        {
            // construct expected Q
            cdrom::SubQ Q_good(Q);
            Q_good.toc.address = cdrom::lba_to_msf(i);
            Q_good.toc.data_address = cdrom::lba_to_msf(i + 150);
            Q_good.crc = endian_swap(crc16_gsm(Q_good.raw, sizeof(Q_good.raw)));

            std::string lc_sector;
            if(Q.toc.address.minute != Q_good.toc.address.minute && Q.toc.data_address.minute != Q_good.toc.data_address.minute &&
               Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
               Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
                lc_sector = "MIN";
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
                    Q.toc.address.second != Q_good.toc.address.second && Q.toc.data_address.second != Q_good.toc.data_address.second &&
                    Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
                lc_sector = "SEC";
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
                    Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
                    Q.toc.address.frame  != Q_good.toc.address.frame  && Q.toc.data_address.frame  != Q_good.toc.data_address.frame)
                lc_sector = "FRM";
            /*
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
            Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
            Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
            lc_sector = "CRC";
            */

            if(!lc_sector.empty())
            {
                uint16_t xor1 = endian_swap(Q_good.crc) ^ endian_swap(Q.crc);
                uint16_t xor2 = crc_le ^ endian_swap(Q.crc);

                // sector
                os << std::setw(5) << msf_to_lba(Q_good.toc.data_address) << '\t';

                os << std::setfill('0') << std::hex;

                // MSF
                os << std::setw(2) << (uint32_t)Q_good.toc.data_address.minute << ':'
                    << std::setw(2) << (uint32_t)Q_good.toc.data_address.second << ':'
                    << std::setw(2) << (uint32_t)Q_good.toc.data_address.frame  << '\t';

                // raw
                for(uint32_t j = 0; j < sizeof(Q.raw); ++j)
                {
                    os << std::setw(2) << (uint32_t)Q.raw[j] << ' ';
                }
                // raw crc
                os << std::setw(2) << (Q.crc & 0xFF) << ' ' << std::setw(2) << (Q.crc >> 8) << '\t';

                // mode
                os << lc_sector << '\t';

                // xor
                os << std::setw(4) << xor1 << ' ' << std::setw(4) << xor2;

                os << std::setfill(' ') << std::dec;
                os << std::endl;
            }

            ofs.write((char *)&Q_good.toc.data_address, sizeof(Q_good.toc.data_address));
            uint8_t p = 1;
            ofs.write((char *)&p, sizeof(p));
            ofs.write((char *)&Q.raw, sizeof(Q.raw));
        }
    }
}


void detect_libcrypt_redumper(std::ostream &os, const std::filesystem::path &sub_file, const std::filesystem::path &sbi_file)
{
    uint32_t file_size = (uint32_t)std::filesystem::file_size(sub_file);
    if(file_size % 96)
        throw_line("subchannel file is incomplete (" + sub_file.generic_string() + ")");

    std::ifstream ifs(sub_file, std::ifstream::binary);
    if(ifs.fail())
        throw_line("unable to open subchannel file (" + sub_file.generic_string() + ")");

    std::ofstream ofs(sbi_file, std::ifstream::binary);
    if(ofs.fail())
        throw_line("unable to create SBI file (" + sbi_file.generic_string() + ")");
    ofs.write("SBI", 4);

    std::vector<uint8_t> buffer(96);
    for(uint32_t i = 0, n = file_size / 96; i < n; ++i)
    {
        ifs.read((char *)buffer.data(), buffer.size());

        cdrom::SubQ Q;
        subcode_extract_channel((uint8_t *)&Q, buffer.data(), cdrom::Subchannel::Q);

        uint16_t crc_le = crc16_gsm(Q.raw, sizeof(Q.raw));
        if(crc_le != endian_swap(Q.crc))
        {
            // construct expected Q
            cdrom::SubQ Q_good(Q);
            Q_good.toc.address = cdrom::lba_to_msf(i);
            Q_good.toc.data_address = cdrom::lba_to_msf(i + 150);
            Q_good.crc = endian_swap(crc16_gsm(Q_good.raw, sizeof(Q_good.raw)));

            std::string lc_sector;
            if(Q.toc.address.minute != Q_good.toc.address.minute && Q.toc.data_address.minute != Q_good.toc.data_address.minute &&
               Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
               Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
                lc_sector = "MIN";
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
                    Q.toc.address.second != Q_good.toc.address.second && Q.toc.data_address.second != Q_good.toc.data_address.second &&
                    Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
                lc_sector = "SEC";
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
                    Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
                    Q.toc.address.frame  != Q_good.toc.address.frame  && Q.toc.data_address.frame  != Q_good.toc.data_address.frame)
                lc_sector = "FRM";
            /*
            else if(Q.toc.address.minute == Q_good.toc.address.minute && Q.toc.data_address.minute == Q_good.toc.data_address.minute &&
            Q.toc.address.second == Q_good.toc.address.second && Q.toc.data_address.second == Q_good.toc.data_address.second &&
            Q.toc.address.frame  == Q_good.toc.address.frame  && Q.toc.data_address.frame  == Q_good.toc.data_address.frame)
            lc_sector = "CRC";
            */

            if(!lc_sector.empty())
            {
                uint16_t xor1 = endian_swap(Q_good.crc) ^ endian_swap(Q.crc);
                uint16_t xor2 = crc_le ^ endian_swap(Q.crc);

                // sector
                os << std::setw(5) << msf_to_lba(Q_good.toc.data_address) << '\t';

                os << std::setfill('0') << std::hex;

                // MSF
                os << std::setw(2) << (uint32_t)Q_good.toc.data_address.minute << ':'
                    << std::setw(2) << (uint32_t)Q_good.toc.data_address.second << ':'
                    << std::setw(2) << (uint32_t)Q_good.toc.data_address.frame  << '\t';

                // raw
                for(uint32_t j = 0; j < sizeof(Q.raw); ++j)
                {
                    os << std::setw(2) << (uint32_t)Q.raw[j] << ' ';
                }
                // raw crc
                os << std::setw(2) << (Q.crc & 0xFF) << ' ' << std::setw(2) << (Q.crc >> 8) << '\t';

                // mode
                os << lc_sector << '\t';

                // xor
                os << std::setw(4) << xor1 << ' ' << std::setw(4) << xor2;

                os << std::setfill(' ') << std::dec;
                os << std::endl;
            }

            ofs.write((char *)&Q_good.toc.data_address, sizeof(Q_good.toc.data_address));
            uint8_t p = 1;
            ofs.write((char *)&p, sizeof(p));
            ofs.write((char *)&Q.raw, sizeof(Q.raw));
        }
    }
}

}
