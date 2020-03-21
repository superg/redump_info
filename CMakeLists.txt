cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME "redump_info")
set(REDUMP_INFO_PROJECT_NAME ${PROJECT_NAME} CACHE STRING "Project name")
project(${REDUMP_INFO_PROJECT_NAME})

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()
# needed to hide other incomplete configurations from Visual Studio
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE INTERNAL "Active configuration" FORCE)
mark_as_advanced(FORCE CMAKE_CONFIGURATION_TYPES)

# installation
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/INSTALL" CACHE PATH "Installation prefix" FORCE)
endif()
set(CMAKE_INSTALL_MESSAGE LAZY)

set(CMAKE_CXX_STANDARD 17)

# Visual Studio settings
if(MSVC)
    # multithreaded build
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# version
string(TIMESTAMP REDUMP_INFO_VERSION_MAJOR "%Y")
string(TIMESTAMP REDUMP_INFO_VERSION_MINOR "%m")
string(TIMESTAMP REDUMP_INFO_TIMESTAMP "%m/%d/%Y %H:%M:%S")
add_definitions(-DREDUMP_INFO_VERSION_MAJOR=${REDUMP_INFO_VERSION_MAJOR}
                -DREDUMP_INFO_VERSION_MINOR=${REDUMP_INFO_VERSION_MINOR}
				-DREDUMP_INFO_TIMESTAMP=${REDUMP_INFO_TIMESTAMP})

# packaging
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${REDUMP_INFO_VERSION_MAJOR}.${REDUMP_INFO_VERSION_MINOR}")
include(CPack)

# testing
enable_testing()

macro(SetTargetCategory target category)
	file(GLOB_RECURSE files LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
    foreach(i IN LISTS files)
        get_filename_component(source_path "${i}" PATH)
        string(REPLACE "/" "\\" source_path "${source_path}")
        source_group("${source_path}" FILES ${i})
    endforeach()
	
	set_property(TARGET ${target} PROPERTY FOLDER ${category})
endmacro()

add_subdirectory("src")
