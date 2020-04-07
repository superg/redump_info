#pragma once



#include <filesystem>
#include <fstream>
#include <string>
#include "cdrom.hh"
#include "iso9660.hh"



namespace redump_info
{

class ImageBrowser
{
public:
	class Entry
	{
		friend class ImageBrowser;

	public:
		bool IsDirectory() const;
		std::list<std::shared_ptr<Entry>> Entries();
		std::shared_ptr<Entry> SubEntry(const std::filesystem::path &path);
		const std::string &Name() const;
		uint32_t Version() const;
		uint32_t Size() const;
		std::string Date() const;
		std::vector<uint8_t> Read(bool form2 = false, bool throw_on_error = false);
		bool IsInterleaved() const;
		//DEBUG
//		std::set<uint8_t> ReadMode2Test();
//		std::vector<uint8_t> Read(uint32_t data_offset, uint32_t size);
//		std::vector<uint8_t> Read(std::set<uint8_t> *xa_channels = NULL);
//		std::vector<uint8_t> ReadXA(uint8_t channel);

	private:
		std::string _name;
		uint32_t _version;
		iso9660::DirectoryRecord _directory_record;
		std::ifstream &_ifs;

		Entry(const std::string &name, uint32_t version, const iso9660::DirectoryRecord &directory_record, std::ifstream &ifs);
	};

	ImageBrowser(const std::filesystem::path &data_track);

	std::shared_ptr<Entry> RootDirectory();

    const iso9660::VolumeDescriptor &GetPVD() const;

private:
	std::ifstream _ifs;
	iso9660::VolumeDescriptor _pvd;
};

}
