/*
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include "cdrom.hh"
#include "disc_browser.hh"
#include "ecc_edc.hh"
#include "version.hh"



//#define MODE2FORM1_EMPTY_ECC_FIX
#define EDCECC_MULTITHREAD



using namespace std;



namespace gpsxre
{

const uint32_t SECTORS_AT_ONCE = 10000;
const uint8_t SECTOR_DATA_SYNC[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

const char antiModStrEn[] =
"     SOFTWARE TERMINATED\nCONSOLE MAY HAVE BEEN MODIFIED\n     CALL 1-888-780-7690";
const char antiModStrJp[] =
"強制終了しました。\n本体が改造されている\nおそれがあります。";


const uint8_t jp[] =
{
    0x8b, 0xad, 0x90, 0xa7, 0x8f, 0x49, 0x97, 0xb9, 0x82, 0xb5, 0x82, 0xdc, 0x82, 0xb5, 0x82, 0xbd, 0x81, 0x42, 0x0a,
    0x96, 0x7b, 0x91, 0xcc, 0x82, 0xaa, 0x89, 0xfc, 0x91, 0xa2, 0x82, 0xb3, 0x82, 0xea, 0x82, 0xc4, 0x82, 0xa2, 0x82, 0xe9, 0x0a,
    0x82, 0xa8, 0x82, 0xbb, 0x82, 0xea, 0x82, 0xaa, 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xdc, 0x82, 0xb7, 0x81, 0x42
};



struct Statistics
{
    std::atomic<uint32_t> m0_count;

    std::atomic<uint32_t> m1_count;
    std::atomic<uint32_t> m1_edc_errors;
    std::atomic<uint32_t> m1_ecc_errors;

    std::atomic<uint32_t> m2_subheader_errors;

    std::atomic<uint32_t> m2f1_count;
    std::atomic<uint32_t> m2f1_edc_errors;
    std::atomic<uint32_t> m2f1_ecc_errors;

    std::atomic<uint32_t> m2f2_count;
    std::atomic<uint32_t> m2f2_edc_errors;
    std::atomic<uint32_t> m2f2_edc_zero;

    std::atomic<uint32_t> mu_count;

    std::atomic<uint32_t> non_data_sectors_count;

    uint32_t total_sectors_count;
};


ostream &operator<<(ostream &os, const Statistics &statistics)
{
    if(statistics.m0_count)
        os << "Mode0 sectors count: " << statistics.m0_count << endl;

    if(statistics.m1_count)
    {
        os << "Mode1 sectors count: " << statistics.m1_count << endl;
        os << "\tEDC errors: " << statistics.m1_edc_errors << endl;
        os << "\tECC errors: " << statistics.m1_ecc_errors << endl;
    }

    if(statistics.m2_subheader_errors)
        os << "Mode2 non-compliant subheader count: " << statistics.m2_subheader_errors << endl;

    if(statistics.m2f1_count)
    {
        os << "Mode2Form1 sectors count: " << statistics.m2f1_count << endl;
        os << "\tEDC errors: " << statistics.m2f1_edc_errors << endl;
        os << "\tECC errors: " << statistics.m2f1_ecc_errors << endl;
    }

    if(statistics.m2f2_count)
    {
        os << "Mode2Form2 sectors count: " << statistics.m2f2_count << endl;
        os << "\tEDC errors: " << statistics.m2f2_edc_errors << endl;
        os << "\tZeroed ECC sectors count: " << statistics.m2f2_edc_zero << endl;
    }

    os << "Total sectors count: " << statistics.total_sectors_count << endl;
    if(statistics.mu_count)
        os << "Unknown mode sectors count: " << statistics.mu_count << endl;
    if(statistics.non_data_sectors_count)
        os << "Non-data sectors count: " << statistics.non_data_sectors_count << endl;
    os << "REDUMP errors: " << (statistics.m1_ecc_errors + statistics.m1_edc_errors +
                               statistics.m2_subheader_errors +
                               statistics.m2f1_ecc_errors + statistics.m2f1_edc_errors +
                               statistics.m2f2_edc_errors) << endl;
    if(statistics.m2f2_count)
        os << "REDUMP EDC: " << (statistics.m2f2_edc_zero == statistics.m2f2_count ? "No" : "Yes") << endl;

    return os;
}


void edc_ecc_check(Statistics &statistics, cdrom::Sector &sector)
{
    if(!equal(begin(sector.sync), end(sector.sync), begin(SECTOR_DATA_SYNC)))
    {
        ++statistics.non_data_sectors_count;
        return;
    }

    switch(sector.header.mode)
    {
    case 0:
        ++statistics.m0_count;
        break;

    case 1:
    {
        ++statistics.m1_count;

        cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
        if(memcmp(ecc.p_parity, sector.mode1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode1.ecc.q_parity, sizeof(ecc.q_parity)))
            ++statistics.m1_ecc_errors;

        uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector, offsetof(cdrom::Sector, mode1.edc));
        if(edc != sector.mode1.edc)
            ++statistics.m1_edc_errors;

        break;
    }

    // XA Mode2 EDC covers subheader, subheader copy and user data, user data size depends on Form1 / Form2 flag
    case 2:
    {
        if(memcmp(&sector.mode2.xa.sub_header, &sector.mode2.xa.sub_header_copy, sizeof(sector.mode2.xa.sub_header)))
            ++statistics.m2_subheader_errors;

        // Form2
        if(sector.mode2.xa.sub_header.submode & (uint8_t)cdrom::CDXAMode::FORM2)
        {
            ++statistics.m2f2_count;

            uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector.mode2.xa.sub_header,
                                              offsetof(cdrom::Sector, mode2.xa.form2.edc) - offsetof(cdrom::Sector, mode2.xa.sub_header));
            if(edc != sector.mode2.xa.form2.edc)
            {
                if(sector.mode2.xa.form2.edc)
                    ++statistics.m2f2_edc_errors;
                else
                    ++statistics.m2f2_edc_zero;
            }
        }
        else
        {
            ++statistics.m2f1_count;

            // EDC
            uint32_t edc = EDC().ComputeBlock(0, (uint8_t *)&sector.mode2.xa.sub_header,
                                              offsetof(cdrom::Sector, mode2.xa.form1.edc) - offsetof(cdrom::Sector, mode2.xa.sub_header));
            if(edc != sector.mode2.xa.form1.edc)
                ++statistics.m2f1_edc_errors;

            // ECC
            // modifies sector, make sure sector data is not used after ECC calculation, otherwise header has to be restored
            cdrom::Sector::Header header = sector.header;
            std::fill_n((uint8_t *)&sector.header, sizeof(sector.header), 0);

            cdrom::Sector::ECC ecc(ECC().Generate((uint8_t *)&sector.header));
            bool error = memcmp(ecc.p_parity, sector.mode2.xa.form1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode2.xa.form1.ecc.q_parity, sizeof(ecc.q_parity));

            // restore modified sector header
            sector.header = header;

#ifdef MODE2FORM1_EMPTY_ECC_FIX
            if(error)
            {
                ecc = ECC().Generate((uint8_t *)&sector.header);
                error = memcmp(ecc.p_parity, sector.mode2.xa.form1.ecc.p_parity, sizeof(ecc.p_parity)) || memcmp(ecc.q_parity, sector.mode2.xa.form1.ecc.q_parity, sizeof(ecc.q_parity));
            }
#endif

            if(error)
                ++statistics.m2f1_ecc_errors;
        }
        break;
    }

    default:
        ++statistics.mu_count;
    }
}


void edc_ecc_check_batch(Statistics &statistics, cdrom::Sector *sectors, uint32_t sectors_count)
{
    for(uint32_t i = 0; i < sectors_count; ++i)
        edc_ecc_check(statistics, sectors[i]);
}


void edc_ecc(const filesystem::path &image_path)
{
    ifstream ifs(image_path, ifstream::binary);
    if(!ifs)
        throw runtime_error("error: unable to open " + image_path.generic_string());

    auto file_size(filesystem::file_size(image_path));
    if(file_size % sizeof(cdrom::Sector))
        throw runtime_error("error: file is not RAW sectors dump");

    Statistics statistics = {};
    statistics.total_sectors_count = (uint32_t)file_size / sizeof(cdrom::Sector);

    auto threads_count(std::thread::hardware_concurrency());

    std::unique_ptr<cdrom::Sector[]> sectors(new cdrom::Sector[SECTORS_AT_ONCE]);
    for(uint32_t sectors_left = (uint32_t)file_size / sizeof(cdrom::Sector); sectors_left; )
    {
        uint32_t sectors_to_process(std::min(SECTORS_AT_ONCE, sectors_left));

        ifs.read((char *)sectors.get(), sectors_to_process * sizeof(cdrom::Sector));

#ifdef EDCECC_MULTITHREAD
        uint32_t sectors_per_thread(sectors_to_process / threads_count + (sectors_to_process % threads_count ? 1 : 0));
        std::vector<future<void>> results(threads_count);
        for(uint32_t i = 0; i < threads_count; ++i)
            results[i] = async(launch::async, edc_ecc_check_batch, reference_wrapper(statistics), &sectors[i * sectors_per_thread],
                               std::min(sectors_per_thread, sectors_to_process - i * sectors_per_thread));

        for(uint32_t i = 0; i < threads_count; ++i)
            results[i].get();
#else
        edc_ecc_check_batch(statistics, &sectors[0], sectors_to_process);
#endif

        sectors_left -= sectors_to_process;

        cout << "\rprogress: " << setw(3) << (statistics.total_sectors_count - sectors_left) * 100 / statistics.total_sectors_count << "%" << flush;
    }
    cout << endl;

    // output results
    cout << "File: " << image_path.generic_string() << endl;
    cout << statistics << endl;
}


void process(const filesystem::path &image_path)
{
    edc_ecc(image_path);



//    cout << "test" << endl;
}

}


int main(int argc, char *argv[])
{
}
*/



#include <iostream>
#include <string>
#include <boost/program_options.hpp>



using namespace std;



namespace po = boost::program_options;



#define xstr(arg_) str(arg_)
#define str(arg_) #arg_
void print_version()
{
    cout << "redump_info v" << RI_VERSION_MAJOR << "." << RI_VERSION_MINOR << " (" << xstr(RI_TIMESTAMP) << ")" << endl;
}


int main(int argc, const char *argv[])
{
    /*
    cout << "test" << endl;

    try {

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("compression", po::value<double>(), "set compression level");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }

        if (vm.count("compression")) {
            cout << "Compression level was set to "
                << vm["compression"].as<double>() << ".\n";
        } else {
            cout << "Compression level was not set.\n";
        }
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
        return 1;
    }

    */
    int exit_code(0);

    try
    {
        print_version();

        po::options_description options("options");
        options.add_options()
            ("help,h", "this help message")
            ("verbose,V", "verbose output")
            ("command,c", po::value<string>(), "info|submission")
            ("input-file", po::value<vector<string>>(), "input file")
            ;
        po::positional_options_description positional_options;
        positional_options.add("input-file", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(options).positional(positional_options).run(), vm);
        po::notify(vm);

        // show usage
        if(vm.count("help"))
        {
            cout << options << endl;
        }
        else
        {
            ;
        }

        cout << "test" << endl;
//        if(argc != 2)
//            throw runtime_error("usage: " + version.BaseName() + " <PSX CD image>");

//        gpsxre::process(argv[1]);
    }
    catch(const exception &e)
    {
        cerr << "error: " << e.what() << endl;
        exit_code = 1;
    }
    catch(...)
    {
        cerr << "error: unhandled exception" << endl;
        exit_code = 2;
    }

    return exit_code;
}
