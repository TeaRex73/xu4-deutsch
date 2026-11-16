#
# Minimal out-of-tree build for SDL_mixer 1.2.x.
#
# Upstream has no CMake build for the SDL-1.2 branch; this file is
# carried here so apple-stuff/build-deps.sh can produce the dylib
# without autotools.  Keep the source list in sync with upstream's
# Makefile.am when the pinned commit changes.
#
cmake_minimum_required(VERSION 3.16)
project(SDL_mixer C)

set(CMAKE_C_STANDARD 99)
include(GNUInstallDirs)

# The superbuild passes these explicitly.  The find_* fallbacks make
# the file usable on its own with CMAKE_PREFIX_PATH set.
set(SDL12_COMPAT_INCLUDE "" CACHE PATH "SDL 1.2 include directory")
set(SDL12_COMPAT_LIBRARY "" CACHE FILEPATH "SDL 1.2 library")
set(OGG_INCLUDE_DIR "" CACHE PATH "libogg include directory")
set(OGG_LIBRARY "" CACHE FILEPATH "libogg library")
set(VORBIS_INCLUDE_DIR "" CACHE PATH "libvorbis include directory")
set(VORBIS_LIBRARY "" CACHE FILEPATH "libvorbis library")
set(VORBISFILE_LIBRARY "" CACHE FILEPATH "libvorbisfile library")

if(NOT SDL12_COMPAT_INCLUDE)
    find_path(SDL12_COMPAT_INCLUDE SDL.h PATH_SUFFIXES SDL)
endif()
if(NOT SDL12_COMPAT_LIBRARY)
    find_library(SDL12_COMPAT_LIBRARY NAMES SDL)
endif()
if(NOT OGG_INCLUDE_DIR)
    find_path(OGG_INCLUDE_DIR ogg/ogg.h)
endif()
if(NOT OGG_LIBRARY)
    find_library(OGG_LIBRARY NAMES ogg)
endif()
if(NOT VORBIS_INCLUDE_DIR)
    find_path(VORBIS_INCLUDE_DIR vorbis/codec.h)
endif()
if(NOT VORBIS_LIBRARY)
    find_library(VORBIS_LIBRARY NAMES vorbis)
endif()
if(NOT VORBISFILE_LIBRARY)
    find_library(VORBISFILE_LIBRARY NAMES vorbisfile)
endif()

foreach(_var
        SDL12_COMPAT_INCLUDE SDL12_COMPAT_LIBRARY
        OGG_INCLUDE_DIR OGG_LIBRARY
        VORBIS_INCLUDE_DIR VORBIS_LIBRARY VORBISFILE_LIBRARY)
    if(NOT ${_var})
        message(FATAL_ERROR "${_var} not set")
    endif()
endforeach()

set(SDL_MIXER_SOURCES
    dynamic_ogg.c
    effect_position.c
    effects_internal.c
    effect_stereoreverse.c
    load_aiff.c
    load_voc.c
    load_ogg.c
    mixer.c
    music.c
    music_ogg.c
)

add_library(SDL_mixer SHARED ${SDL_MIXER_SOURCES})

target_compile_definitions(SDL_mixer PRIVATE _THREAD_SAFE OGG_MUSIC)

target_include_directories(SDL_mixer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL12_COMPAT_INCLUDE}
    ${OGG_INCLUDE_DIR}
    ${VORBIS_INCLUDE_DIR}
)

target_link_libraries(SDL_mixer PRIVATE
    ${SDL12_COMPAT_LIBRARY}
    ${OGG_LIBRARY}
    ${VORBIS_LIBRARY}
    ${VORBISFILE_LIBRARY}
)

set_target_properties(SDL_mixer PROPERTIES
    OUTPUT_NAME "SDL_mixer"
    VERSION "1.2.13"
    SOVERSION "0"
)

install(TARGETS SDL_mixer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES SDL_mixer.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SDL
)
