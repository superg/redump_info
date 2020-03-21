set(includes
#	"${PROJECT_SOURCE_DIR}/utils"
)

set(libs
#	"utils"
)

add_executable(redump_info "main.cc")
target_include_directories(redump_info PUBLIC ${includes})
target_link_libraries(redump_info ${libs})

install(TARGETS redump_info DESTINATION "bin")

SetTargetCategory(redump_info "")
