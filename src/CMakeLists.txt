set(sources
	"cdrom.cc"
	"cdrom.hh"
	"common.hh"
	"endian.cc"
	"endian.hh"
	"image_browser.cc"
	"image_browser.hh"
	"info.cc"
	"info.hh"
	"iso9660.cc"
	"iso9660.hh"
	"options.cc"
	"options.hh"
	"strings.cc"
	"strings.hh"
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
