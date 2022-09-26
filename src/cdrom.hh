#pragma once



#include <cstdint>



namespace cdrom
{

const uint32_t FORM1_DATA_SIZE = 2048;
const uint32_t FORM2_DATA_SIZE = 2324;

struct Sector
{
    struct ECC
    {
        uint8_t p_parity[172];
        uint8_t q_parity[104];
    };

	uint8_t sync[12];

	struct Header
	{
		struct Address
		{
			uint8_t minute;
			uint8_t second;
			uint8_t frame;
		} address;

		uint8_t mode;
	} header;

	union
	{
        struct
        {
            uint8_t user_data[FORM1_DATA_SIZE];
            uint32_t edc;
            uint8_t intermediate[8];
            ECC ecc;
        } mode1;
		struct
		{
			union
			{
				uint8_t user_data[2336];

				struct
				{
					struct SubHeader
					{
						uint8_t file_number;
						uint8_t channel;
						uint8_t submode;
						uint8_t coding_info;
					} sub_header;
					SubHeader sub_header_copy;

					union
					{
						struct
						{
							uint8_t user_data[FORM1_DATA_SIZE];
							uint32_t edc;
							ECC ecc;
						} form1;
						struct
						{
							uint8_t user_data[FORM2_DATA_SIZE];
							uint32_t edc; // reserved
						} form2;
					};
				} xa;
			};
		} mode2;
	};
};

enum class CDXAMode : uint8_t
{
	EORECORD = 1 << 0,
	VIDEO    = 1 << 1,
	AUDIO    = 1 << 2,
	DATA     = 1 << 3,
	TRIGGER  = 1 << 4,
	FORM2    = 1 << 5,
	REALTIME = 1 << 6,
	EOFILE   = 1 << 7
};


enum class Subchannel : uint8_t
{
	P = 7,
	Q = 6,
	R = 5,
	S = 4,
	T = 3,
	U = 2,
	V = 1,
	W = 0
};


struct SubQ
{
    union
    {
        struct
        {
            uint8_t control_adr;

            // mode 1
            struct
            {
                uint8_t track_number;
                uint8_t point;

                Sector::Header::Address address;
                uint8_t reserved;

                union
                {
                    uint8_t data[3];
                    Sector::Header::Address data_address;
                };
            } toc;
        };
        uint8_t raw[10];
    };

    uint16_t crc;
};


template<typename T>
T bcd_decode(T value)
{
    return value / 0x10 * 10 + value % 0x10;
}

template<typename T>
T bcd_encode(T value)
{
    return value / 10 * 0x10 + value % 10;
}

uint32_t msf_to_lba(const Sector::Header::Address &msf);
Sector::Header::Address lba_to_msf(uint32_t lba);
void subcode_extract_channel(uint8_t *subchannel, uint8_t *subcode, Subchannel name);


}
