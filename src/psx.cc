#include <iomanip>
#include <regex>
#include <sstream>
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


std::string extract_serial(ImageBrowser &browser)
{
	std::string serial;

	std::string exe_path = extract_exe_path(browser);

	std::smatch matches;
	std::regex_match(exe_path, matches, std::regex("(.*\\\\)*([A-Z]{4})(_|-)?([0-9]{3})\\.([0-9]{2})"));
	if(matches.size() == 6)
		serial = matches.str(2) + "-" + matches.str(4) + matches.str(5);

	return serial;
}


void detect_anti_modchip_string(std::ostream &os, ImageBrowser &browser)
{
	// taken from DIC
	const char ANTIMOD_MESSAGE_EN[] = "     SOFTWARE TERMINATED\nCONSOLE MAY HAVE BEEN MODIFIED\n     CALL 1-888-780-7690";
	//FIXME: make this more readable
	const uint8_t ANTIMOD_MESSAGE_JP[] =
	{
		0x8b, 0xad, 0x90, 0xa7, 0x8f, 0x49, 0x97, 0xb9, 0x82, 0xb5, 0x82, 0xdc, 0x82, 0xb5, 0x82, 0xbd, 0x81, 0x42, 0x0a,
		0x96, 0x7b, 0x91, 0xcc, 0x82, 0xaa, 0x89, 0xfc, 0x91, 0xa2, 0x82, 0xb3, 0x82, 0xea, 0x82, 0xc4, 0x82, 0xa2, 0x82, 0xe9, 0x0a,
		0x82, 0xa8, 0x82, 0xbb, 0x82, 0xea, 0x82, 0xaa, 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xdc, 0x82, 0xb7, 0x81, 0x42
	};
	//const char ANTIMOD_MESSAGE_JP[] = "強制終了しました。\n本体が改造されている\nおそれがあります。"; // WUT??

	browser.Iterate([&](const std::string &path, std::shared_ptr<ImageBrowser::Entry> d)
	{
		bool exit = false;

		auto fp((path.empty() ? "" : path + "/") + d->Name());

		if(!d->IsInterleaved())
		{
			auto data = d->Read(false, false);

			auto it_en = search(data.begin(), data.end(), std::begin(ANTIMOD_MESSAGE_EN), std::end(ANTIMOD_MESSAGE_EN));
			if(it_en != data.end())
				os << fp << " @ 0x" << std::hex << it_en - data.begin() << std::dec << ": EN" << std::endl;
			auto it_jp = search(data.begin(), data.end(), std::begin(ANTIMOD_MESSAGE_JP), std::end(ANTIMOD_MESSAGE_JP));
			if(it_jp != data.end())
				os << fp << " @ 0x" << std::hex << it_jp - data.begin() << std::dec << ": JP" << std::endl;
		}

		return exit;
	});
}


void detect_libcrypt(std::ostream &os, const std::filesystem::path &sub_file, const std::filesystem::path &sbi_file)
{
    uint32_t file_size = (uint32_t)std::filesystem::file_size(sub_file);
    if(file_size % (sizeof(cdrom::SubQ) * 8))
        throw std::runtime_error("subchannel file is incomplete [" + sub_file.generic_string() + "]");

    std::ifstream ifs(sub_file, std::ifstream::binary);
    if(ifs.fail())
        throw std::runtime_error("unable to open subchannel file [" + sub_file.generic_string() + "]");

    std::ofstream ofs(sbi_file, std::ifstream::binary);
    if(ofs.fail())
        throw std::runtime_error("unable to create SBI file [" + sbi_file.generic_string() + "]");
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

}
