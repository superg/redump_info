#include <fstream>
#include <iostream>
#include <sstream>
#include "cdrom.hh"
#include "common.hh"
#include "crc/Crc32.h"
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
    bool edc;
    string pvd;
    string antimod;
    string libcrypt;
    string dat;
    string cuesheet;
    string write_offset;

    SubmissionInfo()
        : title("(REQUIRED)")
        , foreign_title("(OPTIONAL)")
        , disc_letter("(OPTIONAL)")
        , disc_title("(OPTIONAL)")
        , system("Sony PlayStation")
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
        , edc(false)
        , pvd("(REQUIRED)")
        , antimod("(REQUIRED)")
        , libcrypt("(REQUIRED)")
        , dat("(REQUIRED)")
        , cuesheet("(REQUIRED)")
        , write_offset("(REQUIRED)")
    {
        ;
    }
};

ostream &operator<<(ostream &os, const SubmissionInfo &info)
{
    os << "Common Disc Info:" << endl;
    os << "\tTitle: " << info.title << endl;
    if(!info.foreign_title.empty())
        os << "\tForeign Title (Non-latin): " << info.foreign_title << endl;
    if(!info.disc_letter.empty())
        os << "\tDisc Number / Letter: " << info.disc_letter << endl;
    if(!info.disc_title.empty())
        os << "\tDisc Title: " << info.disc_title << endl;
    os << "\tSystem: " << info.system << endl;
    os << "\tMedia Type: " << info.media_type << endl;
    os << "\tCategory: " << info.category << endl;
    os << "\tRegion: " << info.region << endl;
    os << "\tLanguages: " << info.languages << endl;
    if(!info.disc_serial.empty())
        os << "\tDisc Serial: " << info.disc_serial << endl;
    os << endl;
    os << "\tRingcode Information:" << endl;
    if(!info.mastering_code.empty())
        os << "\t\tMastering Code (laser branded/etched): " << info.mastering_code << endl;
    if(!info.mastering_sid.empty())
        os << "\t\tMastering SID Code: " << info.mastering_sid << endl;
    if(!info.data_mould_sid.empty())
        os << "\t\tData-Side Mould SID Code: " << info.data_mould_sid << endl;
    if(!info.label_mould_sid.empty())
        os << "\t\tLabel-Side Mould SID Code: " << info.label_mould_sid << endl;
    if(!info.additional_mould.empty())
        os << "\t\tAdditional Mould: " << info.additional_mould << endl;
    if(!info.toolstamp.empty())
        os << "\t\tToolstamp or Mastering Code (engraved/stamped): " << info.toolstamp << endl;
    if(!info.barcode.empty())
        os << "\tBarcode: " << info.barcode << endl;
    os << "\tEXE/Build Date: " << info.exe_date << endl;
    os << "\tError Count: " << info.error_count << endl;
    if(!info.comments.empty())
        os << "\tComments: " << info.comments << endl;
    if(!info.contents.empty())
        os << "\tContents: " << info.contents << endl;
    os << endl;
    os << "Version and Editions:" << endl;
    if(!info.version.empty())
        os << "\tVersion: " << info.version << endl;
    os << "\tEdition/Release: " << info.edition << endl;
    os << endl;
    os << "EDC:" << endl;
    os << "\tEDC: " << (info.edc ? "Yes" : "No") << endl;
    os << endl;
    os << "Extras:" << endl;
    os << "\tPrimary Volume Descriptor (PVD):" << endl << endl;
    os << info.pvd << endl;
    os << "Copy Protection:" << endl;
    os << "\tAnti-modchip: ";
    if(info.antimod == "No")
        os << info.antimod << endl;
    else
    {
        os << "Yes" << endl;
        os << info.antimod;
        os << endl;
    }
    os << "\tLibCrypt: ";
    //FIXME: hacky
    if(info.libcrypt == "No" || info.libcrypt == "(REQUIRED)")
        os << info.libcrypt << endl;
    else
    {
        os << "Yes" << endl;
        os << info.libcrypt;
    }

    os << endl;
    os << "Tracks and Write Offsets:" << endl;
    os << "\tDAT:" << endl;
    os << endl;
    os << info.dat << endl;
    os << "\tCuesheet:" << endl;
    os << endl;
    os << info.cuesheet << endl;
    os << "\tWrite Offset: " << info.write_offset << endl;

    return os;
}


const uint32_t SECTORS_AT_ONCE = 10000;


void edc_ecc_check(uint32_t &errors, bool &edc_mode, cdrom::Sector &sector)
{
    switch(sector.header.mode)
    {
    case 1:
    {
        cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
        if(memcmp(ecc.p_parity, sector.mode1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode1.ecc.q_parity, sizeof(ecc.q_parity)))
            ++errors;

        uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector, offsetof(cdrom::Sector, mode1.edc));
        if(edc != sector.mode1.edc)
            ++errors;

        break;
    }

    // XA Mode2 EDC covers subheader, subheader copy and user data, user data size depends on Form1 / Form2 flag
    case 2:
    {
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
            // EDC
            uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector.mode2.xa.sub_header,
                                              offsetof(cdrom::Sector, mode2.xa.form1.edc) - offsetof(cdrom::Sector, mode2.xa.sub_header));
            if(edc != sector.mode2.xa.form1.edc)
                ++errors;

            // ECC
            // modifies sector, make sure sector data is not used after ECC calculation, otherwise header has to be restored
            cdrom::Sector::Header header = sector.header;
            std::fill_n((uint8_t *)&sector.header, sizeof(sector.header), 0);

            cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
            if(memcmp(ecc.p_parity, sector.mode2.xa.form1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode2.xa.form1.ecc.q_parity, sizeof(ecc.q_parity)))
                ++errors;

            // restore modified sector header
            sector.header = header;
        }
        break;
    }

    default:
        ;
    }
}


struct File
{
    string name;
    uint32_t size;
    uint32_t crc;
    string md5;
    string sha1;
};

File create_file_entry(const filesystem::path &p, string name, SubmissionInfo &info, bool data_track)
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
        throw runtime_error("unable to open file " + file_path.generic_string());

    std::unique_ptr<cdrom::Sector[]> sectors(new cdrom::Sector[SECTORS_AT_ONCE]);
    for(uint32_t sectors_left = (uint32_t)size / sizeof(cdrom::Sector); sectors_left; )
    {
        uint32_t sectors_to_process(std::min(SECTORS_AT_ONCE, sectors_left));

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
        info.error_count = std::to_string(errors);
        info.edc = edc_mode;
    }

    return File{name, size, crc, bh_md5.Final(), bh_sha1.Final()};
}


void submission(const Options &o, const std::filesystem::path &p)
{
    try
    {
        cout << p.generic_string() << ": " << endl;

    /*
        // import games from DAT
        list<DAT::Game> games;
        {
            DAT dat(dat_path);
            games = dat.GetGames();
        }

        // build SHA-1 hash map for fast game lookup
        unordered_map<string, std::list<DAT::Game *>> game_lookup;
        for(auto &g : games)
            for(auto &r : g.roms)
                game_lookup[r.sha1].push_back(&g);
    */

        SubmissionInfo info;

        //FIXME: parse command line arguments for mass processing files
    //        info.edition = "Greatest Hits";
    //        info.label_mould_sid = "";
    //        info.additional_mould = "";
    //        info.barcode = "";
    //        info.comments = "";
    //        info.contents = "";
    //        info.version = "";
    //        info.edition = "Original";

        auto cue_files(cue_extract_files(p));

        cout << "\tchecksums calculation... " << flush;
        vector<File> files;
        bool data_track = true;
        for(auto const &e : cue_files)
        {
            files.emplace_back(create_file_entry(p.parent_path(), e, info, data_track));
            data_track = false;
        }
        cout << "done" << endl;

        // data track filesystem routines
        {
            filesystem::path data_track(p.parent_path() / files[0].name);
            ImageBrowser browser(data_track);

            // PVD
            auto pvd = browser.GetPVD();
            info.pvd = hexdump((uint8_t *)&pvd, 0x320, 96);

            // Serial
            string exe_path = psx::extract_exe_path(browser);
            string serial = psx::extract_serial(browser);
            if(serial.empty())
                info.comments = exe_path;
            else
                info.disc_serial = serial;
            
            // exe date
            auto exe_file = browser.RootDirectory()->SubEntry(exe_path);
            if(!exe_file)
                throw runtime_error("unable to access PSX-EXE directory record [" + exe_path + "]");
            info.exe_date = exe_file->Date();

            // antimod
            cout << "\tsearching for anti modchip string... " << flush;
            {
                stringstream ss;
                psx::detect_anti_modchip_string(ss, browser);
                string am_log(ss.str());
                if(am_log.empty())
                    am_log = "No";

                info.antimod = am_log;
            }
            cout << "done" << endl;
        }

        // create basename without using filesystem routines as they mess up dots in path
        string basename = p.generic_string();
        basename.erase(basename.find(".cue"));

        // libcrypt
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

        // DAT
        //FIXME: use tinyxml, "Road & Track Presents - The Need for Speed (USA)" ampersand substitution
        {
            stringstream ss;
            for(auto const &f : files)
            {
                //FIXME: replaces only first occurence
                string name = f.name;
                auto pos = name.find("&");
                if(pos != string::npos)
                    name.replace(pos, 1, "&amp;");

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
                throw runtime_error("unable to open DIC disc file [" + dic_disc_file.generic_string() + "]");

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

/*
        // update submission information from DAT entry, this overrides everything else
        DAT::Game *g = find_game(game_lookup, files);
        if(g != nullptr)
            update_info_from_dat(info, *g);
*/

        // store submission info to game named file
        {
            filesystem::path submission_file(basename);
            submission_file = submission_file.parent_path() / ("!submissionInfo_" + submission_file.filename().string() + ".txt");

            //            filesystem::path submission_file = p;
            //            submission_file.replace_extension("");
            //            submission_file.replace_filename("!submissionInfo_" + submission_file.filename().string());
            //            submission_file.replace_extension(".txt");

            ofstream ofs(submission_file);
            if(ofs.fail())
                throw runtime_error("unable to open output file [" + submission_file.generic_string() + "]");

            ofs << info;
        }


    }
    catch(const std::exception &e)
    {
        if(o.verbose)
            cout << p.generic_string() << ": skipped [" << e.what() << "]" << endl;
    }
}

}
