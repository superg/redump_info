set(sources
	"crc/Crc32.cpp"
	"crc/Crc32.h"
	"tinyxml/tinyxml2.cpp"
	"tinyxml/tinyxml2.h"
	"block_hasher.hh"
	"cdrom.cc"
	"cdrom.hh"
	"common.hh"
	"common.cc"
	"crc16.cc"
	"crc16.hh"
	"dat.cc"
	"dat.hh"
	"ecc_edc.hh"
	"ecc_edc.cc"
	"endian.cc"
	"endian.hh"
	"hex_bin.cc"
	"hex_bin.hh"
	"image_browser.cc"
	"image_browser.hh"
	"info.cc"
	"info.hh"
	"iso9660.cc"
	"iso9660.hh"
	"md5.cc"
	"md5.hh"
	"options.cc"
	"options.hh"
	"psx.cc"
	"psx.hh"
	"sha1.cc"
	"sha1.hh"
	"strings.cc"
	"strings.hh"
	"submission.cc"
	"submission.hh"
	"main.cc"
)

set(includes
#	"${PROJECT_SOURCE_DIR}/utils"
)

set(libs
#	"utils"
)

add_executable(redump_info ${sources})
target_include_directories(redump_info PUBLIC ${includes})
target_link_libraries(redump_info ${libs})

install(TARGETS redump_info DESTINATION "bin")

SetTargetCategory(redump_info "")
