#include <stdexcept>
#include "common.hh"
#include "hex_bin.hh"
#include "strings.hh"
#include "dat.hh"



namespace redump_info
{

DAT::DAT(const std::filesystem::path &dat_file)
{
    if(_doc.LoadFile(dat_file.string().c_str()) != tinyxml2::XML_SUCCESS)
        throw_line(_doc.ErrorStr());

    _datafileNode = _doc.FirstChildElement("datafile");
    if(_datafileNode == nullptr)
        throw_line("error: no xml datafile node");

    LoadGames();
}


std::list<std::pair<std::string, std::string>> DAT::GetHeader() const
{
    // dat header
    std::list<std::pair<std::string, std::string>> header;
    auto header_node = _datafileNode->FirstChildElement("header");
    if(header_node != nullptr)
        for(auto node = header_node->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
            header.push_back(std::pair(std::string(node->Name()), std::string(node->GetText())));

    return header;
}


DAT::Game *DAT::FindGame(const std::list<DAT::Game::Rom> &roms) const
{
    DAT::Game *game = nullptr;

    if(!roms.empty())
    {
        auto it = _gameLookup.find(roms.front().sha1);
        if(it != _gameLookup.end())
        {
            for(auto &g : it->second)
            {
                uint32_t matches = 0;
                for(auto &r : g->roms)
                {
                    // skip cue
                    if(str_lowercase(std::filesystem::path(r.name).extension().generic_string()) == ".cue")
                        continue;

                    for(auto &f : roms)
                    {
                        if(f.size == r.size && f.crc == r.crc && f.md5 == r.md5 && f.sha1 == r.sha1)
                        {
                            ++matches;
                            break;
                        }
                    }
                }

                if(matches == roms.size())
                {
                    game = g;
                    break;
                }
            }
        }
    }

    return game;
}


void DAT::LoadGames()
{
    // dat games
    for(auto game_node = _datafileNode->FirstChildElement("game"); game_node != nullptr; game_node = game_node->NextSiblingElement("game"))
    {
        Game g;

        {
            auto name = game_node->Attribute("name");
            if(name != nullptr)
                g.name = name;

            auto category_node = game_node->FirstChildElement("category");
            if(category_node != nullptr)
            {
                auto category = category_node->GetText();
                if(category != nullptr)
                    g.category = category;
            }

            auto description_node = game_node->FirstChildElement("description");
            if(description_node != nullptr)
            {
                auto description = description_node->GetText();
                if(description != nullptr)
                    g.description = description;
            }
        }

        for(auto rom_node = game_node->FirstChildElement("rom"); rom_node != nullptr; rom_node = rom_node->NextSiblingElement("rom"))
        {
            Game::Rom r;

            auto name = rom_node->Attribute("name");
            if(name != nullptr)
                r.name = name;

            r.size = rom_node->UnsignedAttribute("size");

            auto crc = rom_node->Attribute("crc");
            if(crc != nullptr)
                hex2bin(&r.crc, 1, crc);

            auto md5 = rom_node->Attribute("md5");
            if(md5 != nullptr)
                r.md5 = std::string(md5);

            auto sha1 = rom_node->Attribute("sha1");
            if(sha1 != nullptr)
                r.sha1 = std::string(sha1);

            g.roms.push_back(r);
        }

        _games.push_back(g);
    }

    // build SHA-1 hash map for fast game lookup
    for(auto &g : _games)
        for(auto &r : g.roms)
            _gameLookup[r.sha1].push_back(&g);
}

}
