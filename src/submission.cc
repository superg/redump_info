#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include "cdrom.hh"
#include "common.hh"
#include "crc/Crc32.h"
#include "dat.hh"
#include "ecc_edc.hh"
#include "image_browser.hh"
#include "md5.hh"
#include "psx.hh"
#include "sha1.hh"
#include "strings.hh"
#include "submission.hh"



using namespace std;



namespace redump_info
{

struct SubmissionInfo
{
    string redump_url;
    string title;
    string foreign_title;
    string disc_letter;
    string disc_title;
    string system;
    string media_type;
    string category;
    string region;
    string languages;
    string disc_serial;

    // ring
    string mastering_code;
    string mastering_sid;
    string data_mould_sid;
    string label_mould_sid;
    string additional_mould;
    string toolstamp;

    string barcode;
    string exe_date;
    string error_count;
    string comments;
    string contents;
    string version;
    string edition;
    string edc;
    string pvd;
    string copy_protection;
    string antimod;
    string libcrypt;
    string dat;
    string cuesheet;
    string write_offset;

    SubmissionInfo()
        : redump_url("(REQUIRED, IF VERIFICATION)")
        , title("(REQUIRED)")
        , foreign_title("(OPTIONAL)")
        , disc_letter("(OPTIONAL)")
        , disc_title("(OPTIONAL)")
        , system("(REQUIRED)")
        , media_type("CD-ROM")
        , category("Games")
        , region("World (CHANGE THIS)")
        , languages("Klingon (CHANGE THIS)")
        , disc_serial("(REQUIRED, IF EXISTS)")
        , mastering_code("(REQUIRED, IF EXISTS)")
        , mastering_sid("(REQUIRED, IF EXISTS)")
        , data_mould_sid("(REQUIRED, IF EXISTS)")
        , label_mould_sid("(REQUIRED, IF EXISTS)")
        , additional_mould("(REQUIRED, IF EXISTS)")
        , toolstamp("(REQUIRED, IF EXISTS)")
        , barcode("(OPTIONAL)")
        , exe_date("(REQUIRED)")
        , error_count("(REQUIRED)")
        , comments("(OPTIONAL)")
        , contents("(OPTIONAL)")
        , version("(REQUIRED, IF EXISTS)")
        , edition("Original (VERIFY THIS)")
        , edc("(REQUIRED)")
        , pvd("(REQUIRED)")
        , copy_protection("(REQUIRED)")
        , antimod("(REQUIRED)")
        , libcrypt("(REQUIRED)")
        , dat("(REQUIRED)")
        , cuesheet("(REQUIRED)")
        , write_offset("(REQUIRED)")
    {
        ;
    }

    ostream &operator>>(ostream &os) const
    {
        if(!redump_url.empty())
            os << "Redump URL: " << redump_url << endl;
        os << "Common Disc Info:" << endl;
        os << "\tTitle: " << title << endl;
        if(!foreign_title.empty())
            os << "\tForeign Title (Non-latin): " << foreign_title << endl;
        if(!disc_letter.empty())
            os << "\tDisc Number / Letter: " << disc_letter << endl;
        if(!disc_title.empty())
            os << "\tDisc Title: " << disc_title << endl;
        os << "\tSystem: " << system << endl;
        os << "\tMedia Type: " << media_type << endl;
        os << "\tCategory: " << category << endl;
        os << "\tRegion: " << region << endl;
        if(!languages.empty())
            os << "\tLanguages: " << languages << endl;
        if(!disc_serial.empty())
            os << "\tDisc Serial: " << disc_serial << endl;
        os << endl;

        os << "\tRingcode Information:" << endl;
        if(!mastering_code.empty())
            os << "\t\tMastering Code (laser branded/etched): " << mastering_code << endl;
        if(!mastering_sid.empty())
            os << "\t\tMastering SID Code: " << mastering_sid << endl;
        if(!data_mould_sid.empty())
            os << "\t\tData-Side Mould SID Code: " << data_mould_sid << endl;
        if(!label_mould_sid.empty())
            os << "\t\tLabel-Side Mould SID Code: " << label_mould_sid << endl;
        if(!additional_mould.empty())
            os << "\t\tAdditional Mould: " << additional_mould << endl;
        if(!toolstamp.empty())
            os << "\t\tToolstamp or Mastering Code (engraved/stamped): " << toolstamp << endl;
        if(!barcode.empty())
            os << "\tBarcode: " << barcode << endl;
        if(!exe_date.empty())
            os << "\tEXE/Build Date: " << exe_date << endl;
        if(!error_count.empty())
            os << "\tError Count: " << error_count << endl;
        if(!comments.empty())
            os << "\tComments: " << comments << endl;
        if(!contents.empty())
            os << "\tContents: " << contents << endl;
        os << endl;

        os << "Version and Editions:" << endl;
        if(!version.empty())
            os << "\tVersion: " << version << endl;
        os << "\tEdition/Release: " << edition << endl;
        os << endl;

        if(!edc.empty())
        {
            os << "EDC:" << endl;
            os << "\tEDC: " << edc << endl;
            os << endl;
        }

        if(!pvd.empty())
        {
            os << "Extras:" << endl << "\tPrimary Volume Descriptor (PVD):" << endl;
            os << endl << pvd << endl;
        }

        if(!copy_protection.empty() || !antimod.empty() || !libcrypt.empty())
        {
            os << "Copy Protection:" << endl;
            if(!copy_protection.empty())
            {
                os << "\tCopy Protection: " << copy_protection << endl;
            }

            if(!antimod.empty())
            {
                os << "\tAnti-modchip: ";
                if(antimod == "No" || antimod == "(REQUIRED)")
                    os << antimod << endl;
                else
                {
                    os << "Yes" << endl;
                    os << endl << antimod << endl;
                }
            }

            if(!libcrypt.empty())
            {
                os << "\tLibCrypt: ";
                if(libcrypt == "No" || libcrypt == "(REQUIRED)")
                    os << libcrypt << endl;
                else
                {
                    os << "Yes" << endl;
                    os << endl << libcrypt << endl;
                }
            }
            os << endl;
        }

        os << "Tracks and Write Offsets:" << endl;
        os << "\tDAT:" << endl;
        os << endl;
        os << dat << endl;
        os << "\tCuesheet:" << endl;
        os << endl;
        os << cuesheet << endl;
        os << "\tWrite Offset: " << write_offset << endl;

        return os;
    }
};


const uint32_t SECTORS_AT_ONCE = 10000;


void edc_ecc_check(uint32_t &errors, bool &edc_mode, cdrom::Sector &sector)
{
    switch(sector.header.mode)
    {
    case 1:
    {
        bool error_detected = false;

        cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
        if(memcmp(ecc.p_parity, sector.mode1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode1.ecc.q_parity, sizeof(ecc.q_parity)))
            error_detected = true;

        uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector, offsetof(cdrom::Sector, mode1.edc));
        if(edc != sector.mode1.edc)
            error_detected = true;

        // log dual ECC/EDC mismatch as one error
        if(error_detected)
            ++errors;

        break;
    }

    // XA Mode2 EDC covers subheader, subheader copy and user data, user data size depends on Form1 / Form2 flag
    case 2:
    {
        // subheader mismatch, just a warning
        if(memcmp(&sector.mode2.xa.sub_header, &sector.mode2.xa.sub_header_copy, sizeof(sector.mode2.xa.sub_header)))
            ++errors;

        // Form2
        if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2)
        {
            if(sector.mode2.xa.form2.edc)
            {
                uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector.mode2.xa.sub_header,
                                                  offsetof(cdrom::Sector, mode2.xa.form2.edc) - offsetof(cdrom::Sector, mode2.xa.sub_header));
                if(edc != sector.mode2.xa.form2.edc)
                    ++errors;

                edc_mode = true;
            }
        }
        // Form1
        else
        {
            bool error_detected = false;

            // EDC
            uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector.mode2.xa.sub_header,
                                              offsetof(cdrom::Sector, mode2.xa.form1.edc) - offsetof(cdrom::Sector, mode2.xa.sub_header));
            if(edc != sector.mode2.xa.form1.edc)
                error_detected = true;

            // ECC
            // modifies sector, make sure sector data is not used after ECC calculation, otherwise header has to be restored
            cdrom::Sector::Header header = sector.header;
            fill_n((uint8_t *)&sector.header, sizeof(sector.header), 0);

            cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
            if(memcmp(ecc.p_parity, sector.mode2.xa.form1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode2.xa.form1.ecc.q_parity, sizeof(ecc.q_parity)))
                error_detected = true;

            // restore modified sector header
            sector.header = header;

            // log dual ECC/EDC mismatch as one error
            if(error_detected)
                ++errors;
        }
        break;
    }

    default:
        ;
    }
}


DAT::Game::Rom create_file_entry(const filesystem::path &p, string name, SubmissionInfo &info, bool data_track)
{
    auto file_path(p / name);
    uint32_t size = (uint32_t)filesystem::file_size(file_path);

    uint32_t crc = 0;
    MD5 bh_md5;
    SHA1 bh_sha1;

    uint32_t errors = 0;
    bool edc_mode = false;

    ifstream ifs(file_path, ios::binary);
    if(ifs.fail())
        throw_line("unable to open file (" + file_path.generic_string() + ")");

    unique_ptr<cdrom::Sector[]> sectors(new cdrom::Sector[SECTORS_AT_ONCE]);
    for(uint32_t sectors_left = (uint32_t)size / sizeof(cdrom::Sector); sectors_left; )
    {
        uint32_t sectors_to_process(min(SECTORS_AT_ONCE, sectors_left));

        ifs.read((char *)sectors.get(), sectors_to_process * sizeof(cdrom::Sector));

        crc = crc32_fast(sectors.get(), sectors_to_process * sizeof(cdrom::Sector), crc);
        bh_md5.Update((uint8_t *)sectors.get(), sectors_to_process * sizeof(cdrom::Sector));
        bh_sha1.Update((uint8_t *)sectors.get(), sectors_to_process * sizeof(cdrom::Sector));

        if(data_track)
            for(uint32_t i = 0; i < sectors_to_process; ++i)
                edc_ecc_check(errors, edc_mode, sectors[i]);

        sectors_left -= sectors_to_process;
    }

    if(data_track)
    {
        info.error_count = to_string(errors);
        info.edc = edc_mode ? "Yes" : "No";
    }

    return DAT::Game::Rom{name, size, crc, bh_md5.Final(), bh_sha1.Final()};
}


const unordered_set<string> REGIONS
{
    "Argentina", "Asia", "Australia", "Austria",
    "Belgium", "Brazil",
    "Canada", "China", "Croatia", "Czech",
    "Denmark",
    "Europe",
    "Finland", "France",
    "Germany", "Greater China", "Greece",
    "Hungary",
    "India", "Ireland", "Israel",
    "Italy",
    "Japan",
    "Korea",
    "Latin America",
    "Netherlands", "New Zealand", "Norway",
    "Poland", "Portugal",
    "Russia",
    "Scandinavia", "Singapore", "Slovakia", "South Africa", "Spain", "Sweden", "Switzerland",
    "Taiwan", "Thailand", "Turkey",
    "United Arab Emirate", "UK", "Ukraine", "USA",
    "World"
};

const unordered_map<string, string> LANGUAGES
{
    {"Af", "Afrikaans"},
    {"Ar", "Arabic"},
    {"Eu", "Basque"},
    {"Bg", "Bulgarian"},
    {"Ca", "Catalan"},
    {"Zh", "Chinese"},
    {"Hr", "Croatian"},
    {"Cs", "Czech"},
    {"Da", "Danish"},
    {"Nl", "Dutch"},
    {"En", "English"},
    {"Fi", "Finnish"},
    {"Fr", "French"},
    {"Gd", "Gaelic"},
    {"De", "German"},
    {"El", "Greek"},
    {"Iw", "Hebrew"},
    {"Hi", "Hindi"},
    {"Hu", "Hungarian"},
    {"It", "Italian"},
    {"Ja", "Japanese"},
    {"Ko", "Korean"},
    {"No", "Norwegian"},
    {"Pl", "Polish"},
    {"Pt", "Portuguese"},
    {"Pa", "Punjabi"},
    {"Ro", "Romanian"},
    {"Ru", "Russian"},
    {"Sk", "Slovak"},
    {"Sl", "Slovenian"},
    {"Es", "Spanish"},
    {"Sv", "Swedish"},
    {"Ta", "Tamil"},
    {"Th", "Thai"},
    {"Tr", "Turkish"},
    {"Uk", "Ukrainian"}
};

const unordered_set<string> REGIONS_ENGLISH
{
    "Australia", "Canada", "Europe", "Ireland", "New Zealand", "UK", "USA", "World"
};


// crude but works, everything before region is part of the title
void redump_split_rom_name(string &prefix, string &suffix, const string &rom_name)
{
    prefix = rom_name;
    suffix.clear();

    for(auto const &r : REGIONS)
    {
        auto p = rom_name.find(" (" + r);
        if(p != string::npos)
        {
            prefix = string(rom_name, 0, p);
            suffix = string(rom_name, p + 1);
            break;
        }
    }
}


void update_info_from_dat(SubmissionInfo &info, const DAT::Game &g)
{
    string suffix;
    redump_split_rom_name(info.title, suffix, g.name);
    replace_all_occurences(info.title, " - ", ": ");

    bool multi_disc = false;

    auto options = tokenize_quoted(suffix, " ", "()");
    for(auto const &o : options)
    {
        // comma delimited regions or languages
        auto tokens = tokenize_quoted(o, ",");
        if(tokens.size() > 1)
        {
            set<string> languages;
            for(auto t : tokens)
            {
                trim(t);

                // language list
                auto it = LANGUAGES.find(t);
                if(it != LANGUAGES.end())
                    languages.insert(it->second);
                // region
                else if(REGIONS.find(t) != REGIONS.end())
                {
                    // store the first one
                    info.region = t;
                    break;
                }
            }

            if(!languages.empty())
            {
                bool first_entry = true;
                for(auto const &l : languages)
                {
                    info.languages = first_entry ? l : info.languages + ", " + l;
                    first_entry = false;
                }
            }
        }
        else
        {
            // multi disc
            smatch matches;
            regex_match(o, matches, std::regex("Disc (.*)"));
            if(matches.size() == 2)
            {
                info.disc_letter = matches.str(1);
                multi_disc = true;
            }
            else
            {
                // region
                if(REGIONS.find(o) != REGIONS.end())
                {
                    info.region = o;
                }
                // unlicensed
                else if(o == "Unl")
                {
                    info.edition = "Unlicensed";
                }
                // edition
                else if(!o.find("Demo") || !o.find("Beta"))
                    info.edition = o;
                // version
                else if(o == "Alt" || !o.find("Rev") || o == "EDC" || o == "No EDC")
                    info.version = o;
            }
        }

    }

    if(!multi_disc)
    {
        info.disc_letter.clear();
        info.disc_title.clear();
    }

    info.category = g.category;
}


void submission(const Options &o, const filesystem::path &p, void *data)
{
    try
    {
        auto cue_files(cue_extract_files(p));
        if(cue_files.empty())
            return;

        // accept only tracks with expected extension (.bin)
        // this effectively filters out *_img.cue / *.img files generated by DIC
        // but can be overriden by setting extension option
        string extension("." + str_lowercase(o.extension));
        for(auto const &f : cue_files)
            if(str_lowercase(filesystem::path(f).extension().generic_string()) != extension)
                return;

        cout << p.generic_string() << ": " << endl;

        // create basename without using filesystem routines as they mess up dots in path
        string basename = p.generic_string();
        basename.erase(basename.find(".cue"));

        filesystem::path submission_file(basename);
        submission_file = submission_file.parent_path() / ("!submissionInfo_" + submission_file.filename().string() + ".txt");
        if(filesystem::exists(submission_file) && !o.overwrite)
        {
            if(o.verbose)
                cout << "\tsubmission file already generated, skipping" << endl << endl;

            return;
        }

        SubmissionInfo info;

        filesystem::path data_track_path;

        cout << "\tchecksums calculation... " << flush;
        list<DAT::Game::Rom> roms;
        for(auto const &f : cue_files)
        {
            bool data_track = ImageBrowser::IsDataTrack(p.parent_path() / f);
            if(data_track)
            {
                if(!data_track_path.empty())
                    throw_line("multiple data tracks unsupported");
                data_track_path = p.parent_path() / f;
            }
            roms.emplace_back(create_file_entry(p.parent_path(), f, info, data_track));
        }
        cout << "done" << endl;

        DiscSystem disc_system = data_track_path.empty() ? DiscSystem::AUDIO : DiscSystem::DATA;

        // audio
        if(disc_system == DiscSystem::AUDIO)
        {
            info.system = "Audio CD";
            info.languages.clear();
            info.exe_date.clear();
            info.edc.clear();
            info.error_count.clear();
            info.pvd.clear();
            info.copy_protection.clear();
            info.antimod.clear();
            info.libcrypt.clear();
        }
        // data
        else if(disc_system == DiscSystem::DATA)
        {
            // data track filesystem routines
            filesystem::path data_track(data_track_path);
            ImageBrowser browser(data_track);

            // PVD
            auto pvd = browser.GetPVD();
            info.pvd = hexdump((uint8_t *)&pvd, 0x320, 96);

            // exe path/date and disc system detection
            string exe_path = psx::extract_exe_path(browser);
            auto exe_file = browser.RootDirectory()->SubEntry(exe_path);
            disc_system = (exe_file ? DiscSystem::PSX : DiscSystem::PC);

            switch(disc_system)
            {
            case DiscSystem::DATA:
                ;
                break;

            case DiscSystem::PSX:
            {
                info.system = "Sony PlayStation";
                info.copy_protection.clear();

                {
                    time_t t = exe_file->DateTime();
                    char buffer[32];
                    strftime(buffer, 32, "%Y-%m-%d", localtime(&t));
                    info.exe_date = buffer;
                }

                // serial
                string serial = psx::extract_serial(browser);
                if(serial.empty())
                    info.comments = "Launcher Executable: " + exe_path;
                else
                    info.comments = "Internal Serial: " + serial;
//                    info.disc_serial = serial;

                // region
                string region = psx::extract_region(browser);
                if(!region.empty())
                    info.region = region;

                // antimod
                cout << "\tsearching for anti modchip string... " << flush;
                {
                    auto entries = psx::detect_anti_modchip_string(browser);
                    stringstream ss;
                    for(auto const &e : entries)
                        ss << e << endl;
                    string am_log(ss.str());
                    if(am_log.empty())
                        am_log = "No";

                    info.antimod = am_log;
                }
                cout << "done" << endl;

                // libcrypt
                if(region == "Europe")
                {
                    auto sub_file_path(filesystem::path(basename + ".sub"));
                    if(filesystem::exists(sub_file_path))
                    {
                        stringstream ss;
                        psx::detect_libcrypt(ss, sub_file_path, filesystem::path(basename + ".sbi"));
                        string lc_log(ss.str());
                        if(lc_log.empty())
                            lc_log = "No";

                        info.libcrypt = lc_log;
                    }
                }
                else
                    info.libcrypt = "No";
            }
            break;

            case DiscSystem::PC:
                info.system = "IBM PC Compatible";
                info.comments = "[T:ISBN] (Optional)";
                info.exe_date.clear();
                info.edc.clear();
                info.antimod.clear();
                info.libcrypt.clear();
                break;
            }
        }
        else
        {
            throw_line("unknown system type");
        }


        // DAT
        //FIXME: use tinyxml?
        {
            stringstream ss;
            for(auto const &f : roms)
            {
                string name = f.name;
                replace_all_occurences(name, "&", "&amp;");

                ss << setfill('0') << "<rom name=\"" << name << dec << "\" size=\"" << f.size << hex << "\" crc=\"" << setw(8) << f.crc
                    << "\" md5=\"" << f.md5 << "\" sha1=\"" << f.sha1 << "\" />" << endl;
            }
            info.dat = ss.str();
        }

        // CUE
        {
            ifstream ifs(p);
            stringstream ss;
            string line;
            while(getline(ifs, line))
                ss << line << endl;
            info.cuesheet = ss.str();
        }

        // fetch write offset from DIC output
        if(filesystem::exists(filesystem::path(basename + "_disc.txt")))
        {
            filesystem::path dic_disc_file(basename + "_disc.txt");
            ifstream ifs(dic_disc_file);
            if(ifs.fail())
                throw_line("unable to open DIC disc file (" + dic_disc_file.generic_string() + ")");

            string line;
            while(getline(ifs, line))
            {
                trim(line);
                if(line.rfind("CD Offset", 0) == 0)
                {
                    auto tokens = tokenize_quoted(line, ", ");
                    if(tokens.size() == 5)
                        info.write_offset = tokens[4];
                }
            }
        }

        // fill missing information from DAT file
        auto *dat = reinterpret_cast<DAT *>(data);
        if(dat != nullptr)
        {
            DAT::Game *game = dat->FindGame(roms);

            // game not found (new disc submission or outdated dat file)
            if(game == nullptr)
            {
                info.redump_url.clear();
            }
            // game found (verification)
            else
            {
                update_info_from_dat(info, *game);
            }
        }

        // guess language based on region
        if(info.languages == "Klingon (CHANGE THIS)")
        {
            if(info.region == "Japan" || info.region == "Asia")
                info.languages = "Japanese";
            else if(REGIONS_ENGLISH.find(info.region) != REGIONS_ENGLISH.end())
            {
                info.languages = "English";
                info.foreign_title.clear();
            }
        }

        // override info fields if specified by command line
        if(o.mastering_code)
            info.mastering_code = *o.mastering_code;
        if(o.mastering_sid)
            info.mastering_sid = *o.mastering_sid;
        if(o.data_mould_sid)
            info.data_mould_sid = *o.data_mould_sid;
        if(o.label_mould_sid)
            info.label_mould_sid = *o.label_mould_sid;
        if(o.additional_mould)
            info.additional_mould = *o.additional_mould;
        if(o.toolstamp)
            info.toolstamp = *o.toolstamp;
        if(o.contents)
            info.contents = *o.contents;
        if(o.version)
            info.version = *o.version;
        if(o.edition)
            info.edition = *o.edition;

        // store submission info to game named file
        {
            ofstream ofs(submission_file);
            if(ofs.fail())
                throw_line("unable to open output file (" + submission_file.generic_string() + ")");

            info >> ofs;
        }

        cout << endl;
    }
    catch(const exception &e)
    {
        cout << "{" << e.what() << "}" << endl << endl;
    }
}


void submission_test(const Options &o, const std::filesystem::path &f, void *data)
{
    static set<string> combinations;

    filesystem::path file(f.filename());
    file.replace_extension("");

    string prefix, suffix;
    redump_split_rom_name(prefix, suffix, file.generic_string());

    auto options = tokenize_quoted(suffix, " ", "()");
    for(auto const &o : options)
    {
        auto tokens = tokenize_quoted(o, ",");
        if(tokens.size() == 1 && REGIONS.find(o) == REGIONS.end())
        {
            combinations.insert(o);

            cout << "combinations: " << endl;
            for(auto const &c : combinations)
            {
                cout << c << endl;
            }
            cout << endl;
        }
    }
}

}
