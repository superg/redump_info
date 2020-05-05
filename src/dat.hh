#pragma once



#include <filesystem>
#include <list>
#include <string>
#include <unordered_map>
#include "tinyxml/tinyxml2.h"



namespace redump_info
{

class DAT
{
public:
    struct Game
    {
        std::string name;
        std::string category;
        std::string description;

        struct Rom
        {
            std::string name;
            uint32_t size;
            uint32_t crc;
            std::string md5;
            std::string sha1;
        };

        std::list<Rom> roms;
    };

    DAT(const std::filesystem::path &dat_file);

    std::list<std::pair<std::string, std::string>> GetHeader() const;
    DAT::Game *FindGame(const std::list<DAT::Game::Rom> &roms) const;

private:
    tinyxml2::XMLDocument _doc;
    tinyxml2::XMLElement *_datafileNode;
    std::list<Game> _games;
    std::unordered_map<std::string, std::list<DAT::Game *>> _gameLookup;

    void LoadGames();
};

}
