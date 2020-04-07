#include "cdrom.hh"



namespace cdrom
{

uint32_t msf_to_lba(const Sector::Header::Address &msf)
{
	return 75 * (60 * bcd_decode(msf.minute) + bcd_decode(msf.second)) + bcd_decode(msf.frame);
}


Sector::Header::Address lba_to_msf(uint32_t lba)
{
	Sector::Header::Address address;
	
	address.frame = bcd_encode(lba % 75);
	lba /= 75;
	address.second = bcd_encode(lba % 60);
	lba /= 60;
	address.minute = bcd_encode(lba);

	return address;
}

}
