#include <algorithm>
#include <functional>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "common.hh"
#include "endian.hh"
#include "strings.hh"
#include "image_browser.hh"



#define DIRECTORY_RECORD_WORKAROUNDS

//DEBUG
#include <iostream>



namespace redump_info
{

ImageBrowser::ImageBrowser(const std::filesystem::path &data_track)
	: _ifs(data_track, std::ifstream::binary)
{
	if(_ifs.fail())
		throw_line("unable to open file (" + std::strerror(errno) + ")");

	uint64_t size = std::filesystem::file_size(data_track);

	if(size % sizeof(cdrom::Sector))
		throw_line("file is not a RAW dump (multiple of 2352 bytes)");

    if(size < ((uint64_t)iso9660::SYSTEM_AREA_SIZE + 1) * sizeof(cdrom::Sector))
        throw_line("file is too small");

	// skip system area
	_ifs.seekg(iso9660::SYSTEM_AREA_SIZE * sizeof(cdrom::Sector));
    if(_ifs.fail())
        throw_line("seek failure");

	// find primary volume descriptor
	iso9660::VolumeDescriptor *pvd = nullptr;
	for(;;)
	{
		cdrom::Sector sector;
		_ifs.read((char *)&sector, sizeof(sector));
        if(_ifs.fail())
            break;

        iso9660::VolumeDescriptor *vd;
        switch(sector.header.mode)
        {
        case 1:
            vd = (iso9660::VolumeDescriptor *)sector.mode1.user_data;
            break;

        case 2:
            vd = (iso9660::VolumeDescriptor *)sector.mode2.xa.form1.user_data;
            break;

        default:
            continue;
        }

        if(memcmp(vd->standard_identifier, iso9660::STANDARD_INDENTIFIER, sizeof(vd->standard_identifier)))
            break;

		if(vd->type == iso9660::VolumeDescriptor::Type::PRIMARY)
		{
			pvd = vd;
			break;
		}
		else if(vd->type == iso9660::VolumeDescriptor::Type::SET_TERMINATOR)
			break;
	}

	if(pvd == nullptr)
		throw_line("primary volume descriptor not found");

	_pvd = *pvd;
    _trackSize = (uint32_t)(size / sizeof(cdrom::Sector));
}


std::shared_ptr<ImageBrowser::Entry> ImageBrowser::RootDirectory()
{
	return std::shared_ptr<Entry>(new Entry(*this, std::string(""), 1, _pvd.primary.root_directory_record));
}


const iso9660::VolumeDescriptor &ImageBrowser::GetPVD() const
{
    return _pvd;
}


ImageBrowser::Entry::Entry(ImageBrowser &browser, const std::string &name, uint32_t version, const iso9660::DirectoryRecord &directory_record)
	: _browser(browser)
    , _name(name)
	, _version(version)
	, _directory_record(directory_record)
{
	;
}


bool ImageBrowser::Entry::IsDirectory() const
{
	return _directory_record.file_flags & (uint8_t)iso9660::DirectoryRecord::FileFlags::DIRECTORY;
}


std::list<std::shared_ptr<ImageBrowser::Entry>> ImageBrowser::Entry::Entries()
{
	std::list<std::shared_ptr<ImageBrowser::Entry>> entries;

	if(IsDirectory())
	{
		// read whole directory record to memory
		std::vector<uint8_t> buffer(Read(false, true));

		for(uint32_t i = 0, n = (uint32_t)buffer.size(); i < n;)
		{
			iso9660::DirectoryRecord &dr = *(iso9660::DirectoryRecord *)&buffer[i];

			if(dr.length && dr.length <= cdrom::FORM1_DATA_SIZE - i % cdrom::FORM1_DATA_SIZE)
			{
                // (1) [1/12/2020]: "All Star Racing 2 (Europe) (Track 1).bin"
                // (2) [1/21/2020]: "Aitakute... - Your Smiles in My Heart - Oroshitate no Diary - Introduction Disc (Japan) (Track 1).bin"
                // (3) [1/21/2020]: "MLB 2005 (USA).bin"
                // all these tracks have messed up directory records, (1) and (3) have garbage after valid entries, (2) has garbage before
                // good DirectoryRecord validity trick is to compare lsb to msb for offset and data_length and make sure it's the same
                if(dr.offset.lsb != endian_swap(dr.offset.msb) || dr.data_length.lsb != endian_swap(dr.data_length.msb))
                {
#ifdef DIRECTORY_RECORD_WORKAROUNDS
                    ++i;
                    continue;
#else
                    throw_line("garbage in directory record");
#endif
                }

				char b1 = (char)buffer[i + sizeof(dr)];
				if(b1 != (char)iso9660::Characters::DIR_CURRENT && b1 != (char)iso9660::Characters::DIR_PARENT)
				{
					std::string identifier((const char *)& buffer[i + sizeof(dr)], dr.file_identifier_length);
					auto s = identifier.find((char)iso9660::Characters::SEPARATOR2);
					std::string name(s == std::string::npos ? identifier : identifier.substr(0, s));

					uint32_t version(s == std::string::npos ? 1 : std::stoi(identifier.substr(s + 1)));

//					entries.push_back(std::make_shared<Entry>(name, version, dr, _ifs));
                    entries.push_back(std::shared_ptr<Entry>(new Entry(_browser, name, version, dr)));
				}

				i += dr.length;
			}
			// skip sector boundary
			else
				i = ((i / cdrom::FORM1_DATA_SIZE) + 1) * cdrom::FORM1_DATA_SIZE;
		}
	}

	return entries;
}


std::shared_ptr<ImageBrowser::Entry> ImageBrowser::Entry::SubEntry(const std::filesystem::path &path)
{
	std::shared_ptr<Entry> entry;

    std::filesystem::path path_case(str_uppercase(path.generic_string()));

    for(auto const &p : path_case)
    {
        auto directories = entry ? entry->Entries() : Entries();
        bool found = false;
        for(auto &d : directories)
        {
            std::string name_case(str_uppercase(d->Name()));
            if(name_case == p || (name_case + ';' + std::to_string(d->Version())) == p)
            {
                entry = d;
                found = true;
                break;
            }
        }

        if(!found)
        {
            entry.reset();
            break;
        }
    }

	return entry;
}


const std::string &ImageBrowser::Entry::Name() const
{
	return _name;
}


uint32_t ImageBrowser::Entry::Version() const
{
	return _version;
}


bool ImageBrowser::Entry::IsDummy() const
{
    return _directory_record.offset.lsb + SectorSize() >= _browser._trackSize || _directory_record.offset.lsb >= _browser._trackSize;
}


uint32_t ImageBrowser::Entry::SectorSize() const
{
    return _directory_record.data_length.lsb / cdrom::FORM1_DATA_SIZE
        + (_directory_record.data_length.lsb % cdrom::FORM1_DATA_SIZE ? 1 : 0);
}


std::string ImageBrowser::Entry::Date() const
{
    // Chrono Cross (USA) (Disc 1)
    uint32_t year = _directory_record.recording_date_time.year;
    if(year >= 0 && year <= 20)
        year += 100;

    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(4) << year + 1900 << '-'
       << std::setw(2) << (uint32_t)_directory_record.recording_date_time.month << '-'
       << std::setw(2) << (uint32_t)_directory_record.recording_date_time.day;

    return ss.str();
}


std::vector<uint8_t> ImageBrowser::Entry::Read(bool form2, bool throw_on_error)
{
    std::vector<uint8_t> data;

    uint32_t size = _directory_record.data_length.lsb;
    data.reserve(size);

    _browser._ifs.seekg(_directory_record.offset.lsb * sizeof(cdrom::Sector));
    if(_browser._ifs.fail())
    {
        _browser._ifs.clear();
        if(throw_on_error)
            throw_line("seek failure");
    }
    else
    {
        uint32_t sectors_count = SectorSize();
        for(uint32_t s = 0; s < sectors_count; ++s)
        {
            cdrom::Sector sector;
            _browser._ifs.read((char *)&sector, sizeof(sector));
            if(_browser._ifs.fail())
            {
                auto message(std::string("read failure [") + std::strerror(errno) + "]");
                _browser._ifs.clear();
                if(throw_on_error)
                    throw_line(message);

                break;
            }

            uint8_t *user_data;
            uint32_t bytes_to_copy;
            if(sector.header.mode == 1)
            {
                if(form2)
                    continue;

                user_data = sector.mode1.user_data;
                bytes_to_copy = std::min(cdrom::FORM1_DATA_SIZE, size);
            }
            else if(sector.header.mode == 2)
            {
                if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2)
                {
                    if(!form2)
                        continue;

                    user_data = sector.mode2.xa.form2.user_data;
                    bytes_to_copy = size < cdrom::FORM1_DATA_SIZE ? size : cdrom::FORM2_DATA_SIZE;
                }
                else
                {
                    if(form2)
                        continue;

                    user_data = sector.mode2.xa.form1.user_data;
                    bytes_to_copy = std::min(cdrom::FORM1_DATA_SIZE, size);
                }
            }
            else
                continue;

            data.insert(data.end(), user_data, user_data + bytes_to_copy);

            size -= std::min(cdrom::FORM1_DATA_SIZE, size);
        }
    }

    return data;
}


bool ImageBrowser::Entry::IsInterleaved() const
{
    bool interleaved = false;

    static const uint32_t SECTORS_TO_ANALYZE = 8 * 4;

    _browser._ifs.seekg(_directory_record.offset.lsb * sizeof(cdrom::Sector));
    if(_browser._ifs.fail())
    {
        _browser._ifs.clear();
        throw_line("seek failure");
    }

    uint8_t file_form = 0;

    uint32_t size = _directory_record.data_length.lsb;
    uint32_t sectors_count = std::min(SectorSize(), SECTORS_TO_ANALYZE);
    for(uint32_t s = 0; s < sectors_count; ++s)
    {
        cdrom::Sector sector;
        _browser._ifs.read((char *)&sector, sizeof(sector));
        if(_browser._ifs.fail())
        {
            auto message(std::string("read failure (") + std::strerror(errno) + ")");
            _browser._ifs.clear();
            throw_line(message);
        }

        uint8_t file_form_next = 0;
        if(sector.header.mode == 1)
            file_form_next = 1;
        else if(sector.header.mode == 2)
            file_form_next = sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2 ? 2 : 1;

        if(file_form)
        {
            if(file_form != file_form_next)
            {
                interleaved = true;
                break;
            }
        }
        else
            file_form = file_form_next;
    }

    return interleaved;
}


/*
// this is sequential as interleaved sectors have to be taken into account
std::vector<uint8_t> ImageBrowser::Entry::Read(uint32_t data_offset, uint32_t size)
{
std::vector<uint8_t> data;
data.reserve(size);

uint32_t sectors_count = _directory_record.data_length.lsb / cdrom::FORM1_DATA_SIZE + (_directory_record.data_length.lsb % cdrom::FORM1_DATA_SIZE ? 1 : 0);

uint32_t sector_offset = data_offset / cdrom::FORM1_DATA_SIZE;
uint32_t offset = data_offset % cdrom::FORM1_DATA_SIZE;

for(uint32_t s = 0, ds = 0; s < sectors_count; ++s)
{
cdrom::Sector sector;
_ifs.read((char *)&sector, sizeof(sector));

// skip interleaved audio sectors
if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::AUDIO)
continue;

// PSX CDXA Mode2Form2 is used only for audio data
if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2)
throw_line("Mode2Form2 non audio sector detected");

if(ds >= sector_offset)
{
uint32_t bytes_to_copy = std::min(cdrom::FORM1_DATA_SIZE - offset, size);

auto data_start = sector.mode2.xa.form1.user_data + offset;
data.insert(data.end(), data_start, data_start + bytes_to_copy);
offset = 0;
size -= bytes_to_copy;

if(!size)
break;
}

++ds;
}

return data;
}


std::vector<uint8_t> ImageBrowser::Entry::Read(std::set<uint8_t> *xa_channels)
{
    std::vector<uint8_t> data;

    uint32_t size = _directory_record.data_length.lsb;
    data.reserve(size);

    _ifs.seekg(_directory_record.offset.lsb * sizeof(cdrom::Sector));
    if(_ifs.fail())
    {
        std::cout << std::strerror(errno) << std::endl;
        throw_line("fff");
    }

    uint32_t sectors_count = size / cdrom::FORM1_DATA_SIZE + (size % cdrom::FORM1_DATA_SIZE ? 1 : 0);
    for(uint32_t s = 0; s < sectors_count; ++s)
    {
        cdrom::Sector sector;
        _ifs.read((char *)&sector, sizeof(sector));
    if(_ifs.fail())
    {
        std::cout << std::strerror(errno) << std::endl;
        throw_line("fff");
    }

        // skip interleaved audio sectors
        if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::AUDIO)
        {
            if(xa_channels != nullptr)
                xa_channels->insert(sector.mode2.xa.sub_header.channel);
        }
        else
        {
            // PSX CDXA Mode2Form2 is used only for audio data
            if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2)
                throw_line("Mode2Form2 non audio sector detected");

            uint32_t bytes_to_copy = std::min(cdrom::FORM1_DATA_SIZE, size);
            data.insert(data.end(), sector.mode2.xa.form1.user_data, sector.mode2.xa.form1.user_data + bytes_to_copy);

            size -= bytes_to_copy;
        }
    }

    return data;
}


std::vector<uint8_t> ImageBrowser::Entry::ReadXA(uint8_t channel)
{
	std::vector<uint8_t> data;

	_ifs.seekg(_directory_record.offset.lsb * sizeof(cdrom::Sector));
    if(_ifs.fail())
    {
        std::cout << std::strerror(errno) << std::endl;
        throw_line("fff");
    }

	uint32_t sectors_count = _directory_record.data_length.lsb / cdrom::FORM1_DATA_SIZE + (_directory_record.data_length.lsb % cdrom::FORM1_DATA_SIZE ? 1 : 0);
	for(uint32_t s = 0; s < sectors_count; ++s)
	{
		cdrom::Sector sector;
		_ifs.read((char *)&sector, sizeof(sector));
    if(_ifs.fail())
    {
        std::cout << std::strerror(errno) << std::endl;
        throw_line("fff");
    }

		if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::AUDIO && sector.mode2.xa.sub_header.channel == channel)
			data.insert(data.end(), sector.mode2.xa.form2.user_data, sector.mode2.xa.form2.user_data + cdrom::FORM2_DATA_SIZE);
	}

	return data;
}
*/

}
