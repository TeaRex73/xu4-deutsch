cmake_minimum_required(VERSION 3.16)

# MACOSX_DEPLOYMENT_TARGET on a target is only honoured by the Xcode
# generator.  Set the cache variable here so every generator (and
# CLion's default profile) produces binaries that run on 10.13.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING
      "Minimum supported macOS version")
endif()

project(xu4-deutsch LANGUAGES C CXX)

# Build date shown by the game.  The macOS release script passes the
# date explicitly; other CMake builds fall back to today's UTC date.
# A future release pipeline can hand in the date of the final included
# commit.
if(NOT XU4_BUILD_DATE)
  string(TIMESTAMP XU4_BUILD_DATE "%Y-%m-%d" UTC)
endif()

include(GNUInstallDirs)

option(XU4_BUILD_UTILS "Build utility tools" ON)

set(XU4_UI "sdl" CACHE STRING "UI backend (sdl)")
set_property(CACHE XU4_UI PROPERTY STRINGS sdl)

if(APPLE)
  set(XU4_GIT_DEPS_DIR "$ENV{HOME}/git" CACHE PATH "Base directory for locally built dependencies")
  set(XU4_DEPS_PREFIX "" CACHE PATH "Prefix with the vendored macOS dependencies")

  string(TOLOWER "${CMAKE_BUILD_TYPE}" _xu4_build_type_lc)
  set(_xu4_dep_configs "")
  if(_xu4_build_type_lc STREQUAL "debug")
    list(APPEND _xu4_dep_configs debug relwithdebinfo release minsizerel default)
  elseif(_xu4_build_type_lc STREQUAL "release")
    list(APPEND _xu4_dep_configs release relwithdebinfo minsizerel default debug)
  elseif(_xu4_build_type_lc STREQUAL "minsizerel")
    list(APPEND _xu4_dep_configs minsizerel release relwithdebinfo default debug)
  elseif(_xu4_build_type_lc STREQUAL "relwithdebinfo")
    list(APPEND _xu4_dep_configs relwithdebinfo release minsizerel default debug)
  else()
    list(APPEND _xu4_dep_configs relwithdebinfo release minsizerel default debug)
  endif()

  file(GLOB _xu4_sdl12compat_roots LIST_DIRECTORIES true
       "${XU4_GIT_DEPS_DIR}/sdl12-compat-release-*")
  list(SORT _xu4_sdl12compat_roots ORDER DESCENDING)
  set(XU4_SDL12COMPAT_HINTS "")
  foreach(_root IN LISTS _xu4_sdl12compat_roots)
    foreach(_cfg IN LISTS _xu4_dep_configs)
      list(APPEND XU4_SDL12COMPAT_HINTS
        "${_root}/cmake-build-${_cfg}-xcode-26")
    endforeach()
  endforeach()

  file(GLOB _xu4_libpng_roots LIST_DIRECTORIES true
       "${XU4_GIT_DEPS_DIR}/libpng-*")
  list(SORT _xu4_libpng_roots ORDER DESCENDING)
  set(XU4_LIBPNG_HINTS "")
  foreach(_root IN LISTS _xu4_libpng_roots)
    foreach(_cfg IN LISTS _xu4_dep_configs)
      list(APPEND XU4_LIBPNG_HINTS
        "${_root}/cmake-build-${_cfg}-xcode-26")
    endforeach()
    list(APPEND XU4_LIBPNG_HINTS "${_root}")
  endforeach()

  set(XU4_SDLMIXER_HINTS "")
  foreach(_cfg IN LISTS _xu4_dep_configs)
    list(APPEND XU4_SDLMIXER_HINTS
      "${XU4_GIT_DEPS_DIR}/SDL_mixer/cmake-build-${_cfg}-xcode-26")
  endforeach()

  # A self-contained prefix from apple-stuff/build-deps.sh takes
  # precedence over the per-checkout build trees above.
  if(XU4_DEPS_PREFIX)
    list(APPEND XU4_SDL12COMPAT_HINTS
      "${XU4_DEPS_PREFIX}/bin")
    list(APPEND XU4_LIBPNG_HINTS
      "${XU4_DEPS_PREFIX}/include" "${XU4_DEPS_PREFIX}/lib")
    list(APPEND XU4_SDLMIXER_HINTS
      "${XU4_DEPS_PREFIX}/lib")
  endif()

  # Prefer dependencies under $HOME/git and SDK/system locations over /opt (e.g. MacPorts).
  list(APPEND CMAKE_PREFIX_PATH
    ${XU4_SDL12COMPAT_HINTS}
    ${XU4_SDLMIXER_HINTS}
    ${XU4_LIBPNG_HINTS}
  )
  if(XU4_DEPS_PREFIX)
    list(APPEND CMAKE_PREFIX_PATH "${XU4_DEPS_PREFIX}")
  endif()
  set(CMAKE_IGNORE_PREFIX_PATH "/opt/local;/opt/homebrew;/opt")
endif()

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(PkgConfig QUIET)
find_package(Threads REQUIRED)

if(APPLE)
  # macOS system/SDK libraries where available
  find_library(LIBXML2_LIBRARY NAMES xml2
               HINTS "${CMAKE_OSX_SYSROOT}/usr/lib" /usr/lib
               NO_CMAKE_FIND_ROOT_PATH)
  find_path(LIBXML2_INCLUDE_DIR libxml/parser.h
            HINTS "${CMAKE_OSX_SYSROOT}/usr/include/libxml2" /usr/include/libxml2
            NO_CMAKE_FIND_ROOT_PATH)
  if(NOT LIBXML2_LIBRARY OR NOT LIBXML2_INCLUDE_DIR)
    message(FATAL_ERROR "macOS libxml2 (SDK/system) not found")
  endif()
  set(LIBXML2_LIBRARIES "${LIBXML2_LIBRARY}")
  set(LIBXML2_INCLUDE_DIRS "${LIBXML2_INCLUDE_DIR}")
  set(LIBXML2_FOUND TRUE)

  find_library(ZLIB_LIBRARY NAMES z
               HINTS "${CMAKE_OSX_SYSROOT}/usr/lib" /usr/lib
               NO_CMAKE_FIND_ROOT_PATH)
  find_path(ZLIB_INCLUDE_DIR zlib.h
            HINTS "${CMAKE_OSX_SYSROOT}/usr/include" /usr/include
            NO_CMAKE_FIND_ROOT_PATH)
  if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
    message(FATAL_ERROR "macOS zlib (SDK/system) not found")
  endif()
  set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}")
  set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
  set(ZLIB_FOUND TRUE)

  # Non-system dependencies from $HOME/git
  find_path(PNG_PNG_INCLUDE_DIR png.h
            HINTS ${XU4_LIBPNG_HINTS}
            NO_DEFAULT_PATH)
  find_library(PNG_LIBRARY NAMES png16 libpng16 png
               HINTS ${XU4_LIBPNG_HINTS}
               NO_DEFAULT_PATH)
  if(NOT PNG_PNG_INCLUDE_DIR OR NOT PNG_LIBRARY)
    message(FATAL_ERROR "libpng not found under ${XU4_GIT_DEPS_DIR}/libpng-*")
  endif()
  set(PNG_INCLUDE_DIRS "${PNG_PNG_INCLUDE_DIR}")
  set(PNG_LIBRARIES "${PNG_LIBRARY}")
  set(PNG_FOUND TRUE)
else()
  find_package(LibXml2 REQUIRED)
  find_package(PNG REQUIRED)
  find_package(ZLIB REQUIRED)
endif()

set(XU4_SDL_INCLUDE_DIRS "")
set(XU4_SDL_LIBRARIES "")
set(XU4_SDLMIXER_INCLUDE_DIRS "")
set(XU4_SDLMIXER_LIBRARIES "")

if(APPLE)
  find_program(SDL_CONFIG_EXECUTABLE NAMES sdl-config sdl12-config
               HINTS ${XU4_SDL12COMPAT_HINTS}
               NO_DEFAULT_PATH)
else()
  find_program(SDL_CONFIG_EXECUTABLE NAMES sdl-config sdl12-config)
endif()

if(SDL_CONFIG_EXECUTABLE)
  execute_process(COMMAND ${SDL_CONFIG_EXECUTABLE} --cflags
                  OUTPUT_VARIABLE _sdl_cflags OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${SDL_CONFIG_EXECUTABLE} --libs
                  OUTPUT_VARIABLE _sdl_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(_sdl_cflags NATIVE_COMMAND "${_sdl_cflags}")
  separate_arguments(_sdl_libs NATIVE_COMMAND "${_sdl_libs}")
  foreach(_flag IN LISTS _sdl_cflags)
    if(_flag MATCHES "^-I(.+)$")
      list(APPEND XU4_SDL_INCLUDE_DIRS "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  set(XU4_SDL_LIBRARIES ${_sdl_libs})
  set(XU4_SDL_FOUND TRUE)
elseif(PKG_CONFIG_FOUND)
  pkg_check_modules(XU4_SDL QUIET sdl)
endif()

if(NOT XU4_SDL_FOUND)
  find_package(SDL QUIET)
  if(SDL_FOUND)
    set(XU4_SDL_INCLUDE_DIRS "${SDL_INCLUDE_DIR}")
    set(XU4_SDL_LIBRARIES "${SDL_LIBRARY}")
  endif()
endif()

if(NOT XU4_SDL_FOUND AND NOT XU4_SDL_LIBRARIES)
  message(FATAL_ERROR "SDL 1.2 development files not found")
endif()

if(PKG_CONFIG_FOUND AND NOT APPLE)
  pkg_check_modules(XU4_SDLMIXER QUIET SDL_mixer)
endif()

if(NOT XU4_SDLMIXER_FOUND)
  if(APPLE)
    find_library(XU4_SDLMIXER_LIBRARY NAMES SDL_mixer
                 HINTS ${XU4_SDLMIXER_HINTS}
                 NO_DEFAULT_PATH)
    find_path(XU4_SDLMIXER_INCLUDE_DIR SDL_mixer.h
              HINTS
                "${XU4_GIT_DEPS_DIR}/SDL_mixer"
                "${XU4_GIT_DEPS_DIR}/SDL_mixer/include"
                "${XU4_DEPS_PREFIX}/include/SDL"
              NO_DEFAULT_PATH)
  else()
    find_library(XU4_SDLMIXER_LIBRARY NAMES SDL_mixer)
    find_path(XU4_SDLMIXER_INCLUDE_DIR SDL_mixer.h
              PATH_SUFFIXES SDL SDL1 include/SDL include/SDL1)
  endif()
  if(XU4_SDLMIXER_LIBRARY AND XU4_SDLMIXER_INCLUDE_DIR)
    set(XU4_SDLMIXER_LIBRARIES "${XU4_SDLMIXER_LIBRARY}")
    set(XU4_SDLMIXER_INCLUDE_DIRS "${XU4_SDLMIXER_INCLUDE_DIR}")
    set(XU4_SDLMIXER_FOUND TRUE)
  endif()
endif()

if(NOT XU4_SDLMIXER_FOUND)
  message(FATAL_ERROR "SDL_mixer development files not found")
endif()

set(XU4_FEATURE_DEFS
  HAVE_BACKTRACE=1
  HAVE_VARIADIC_MACROS=1
)

if(APPLE)
  list(APPEND XU4_FEATURE_DEFS _DARWIN_C_SOURCE)
  set(XU4_INSTALL_LIBSUBDIR "u4g")
else()
  set(XU4_INSTALL_LIBSUBDIR "u4")
endif()

set(XU4_WARNING_FLAGS
  -Wall -Wextra -Werror
  -Wmissing-declarations
  -Wno-deprecated-declarations
)

if(APPLE)
  list(APPEND XU4_WARNING_FLAGS -Wno-nullability-completeness)
else()
  list(APPEND XU4_WARNING_FLAGS -Wlogical-op)
endif()

set(XU4_COMMON_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${XU4_SDL_INCLUDE_DIRS}
  ${XU4_SDLMIXER_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${PNG_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
)

if(APPLE)
  set(XU4_COMMON_LIBS
    Threads::Threads
    ${XU4_SDLMIXER_LIBRARIES}
    ${XU4_SDL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
  )
else()
  set(XU4_COMMON_LIBS
    Threads::Threads
    ${XU4_SDLMIXER_LIBRARIES}
    ${XU4_SDL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    PNG::PNG
    ZLIB::ZLIB
  )
endif()

set(XU4_COMMON_DEFS
  ${XU4_FEATURE_DEFS}
  _GNU_SOURCE=1
  ICON_FILE="${CMAKE_INSTALL_FULL_DATADIR}/pixmaps/u4.bmp"
)

set(XU4_CSRCS
  src/lzw/hash.c
  src/lzw/lzw.c
  src/unzip.c
)

set(XU4_CXXSRCS
  src/annotation.cpp
  src/armor.cpp
  src/aura.cpp
  src/camp.cpp
  src/cheat.cpp
  src/city.cpp
  src/codex.cpp
  src/combat.cpp
  src/config.cpp
  src/controller.cpp
  src/context.cpp
  src/conversation.cpp
  src/creature.cpp
  src/death.cpp
  src/debug.cpp
  src/dialogueloader.cpp
  src/dialogueloader_hw.cpp
  src/dialogueloader_lb.cpp
  src/dialogueloader_tlk.cpp
  src/direction.cpp
  src/dungeon.cpp
  src/dungeonview.cpp
  src/error.cpp
  src/event.cpp
  src/event_${XU4_UI}.cpp
  src/filesystem.cpp
  src/game.cpp
  src/gamedata.cpp
  src/image_${XU4_UI}.cpp
  src/imageloader.cpp
  src/imageloader_png.cpp
  src/imageloader_u4.cpp
  src/imageloader_u5.cpp
  src/imageloader_fmtowns.cpp
  src/imagemgr.cpp
  src/imageview.cpp
  src/intro.cpp
  src/item.cpp
  src/location.cpp
  src/map.cpp
  src/maploader.cpp
  src/mapmgr.cpp
  src/menu.cpp
  src/menuitem.cpp
  src/moongate.cpp
  src/movement.cpp
  src/music.cpp
  src/music_${XU4_UI}.cpp
  src/names.cpp
  src/object.cpp
  src/person.cpp
  src/player.cpp
  src/portal.cpp
  src/progress_bar.cpp
  src/rle.cpp
  src/savegame.cpp
  src/scale.cpp
  src/script.cpp
  src/screen.cpp
  src/screen_${XU4_UI}.cpp
  src/settings.cpp
  src/shrine.cpp
  src/sound.cpp
  src/sound_${XU4_UI}.cpp
  src/spell.cpp
  src/stats.cpp
  src/textview.cpp
  src/tile.cpp
  src/tileanim.cpp
  src/tilemap.cpp
  src/tileset.cpp
  src/tileview.cpp
  src/u4.cpp
  src/u4_${XU4_UI}.cpp
  src/u4file.cpp
  src/u4io.cpp
  src/utils.cpp
  src/view.cpp
  src/weapon.cpp
  src/xml.cpp
  src/xordata.cpp
  src/lzw/u4decode.cpp
  src/lzw/u6decode.cpp
)

add_executable(u4 ${XU4_CSRCS} ${XU4_CXXSRCS})
target_include_directories(u4 PRIVATE ${XU4_COMMON_INCLUDE_DIRS})
target_link_libraries(u4 PRIVATE ${XU4_COMMON_LIBS})
target_compile_definitions(u4 PRIVATE
  ${XU4_COMMON_DEFS}
  VERSION="${XU4_BUILD_DATE}"
)
target_compile_options(u4 PRIVATE
  ${XU4_WARNING_FLAGS}
  $<$<COMPILE_LANGUAGE:CXX>:-Weffc++ -Wold-style-cast -Wsign-promo -Wsuggest-override>
)

if(APPLE)
  set_target_properties(u4 PROPERTIES
    MACOSX_DEPLOYMENT_TARGET "10.13"
  )
endif()

function(xu4_add_tool name)
  cmake_parse_arguments(XU4_TOOL "" "" "SOURCES;LIBS" ${ARGN})
  add_executable(${name} ${XU4_TOOL_SOURCES})
  target_include_directories(${name} PRIVATE ${XU4_COMMON_INCLUDE_DIRS})
  target_link_libraries(${name} PRIVATE ${XU4_TOOL_LIBS})
  target_compile_definitions(${name} PRIVATE ${XU4_COMMON_DEFS})
  target_compile_options(${name} PRIVATE
    ${XU4_WARNING_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:-Weffc++ -Wold-style-cast -Wsign-promo -Wsuggest-override>
  )
  if(APPLE)
    set_target_properties(${name} PROPERTIES
      MACOSX_DEPLOYMENT_TARGET "10.13"
    )
  endif()
endfunction()

if(XU4_BUILD_UTILS)
  xu4_add_tool(coord SOURCES src/util/coord.c LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(dumpsavegame
    SOURCES src/util/dumpsavegame.cpp src/savegame.cpp src/u4io.cpp src/names.cpp
    LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(recreate
    SOURCES src/util/recreate.cpp src/u4file.cpp src/unzip.c src/xordata.cpp
    LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(tlkconv SOURCES src/util/tlkconv.c LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(u4dec
    SOURCES src/util/u4dec.c src/lzw/lzw.c src/lzw/hash.c src/rle.cpp src/util/pngconv.c
    LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(u4enc
    SOURCES src/util/u4enc.c src/lzw/hash.c src/util/pngconv.c
    LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(u4unpackexe SOURCES src/util/u4unpackexe.c LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(xor SOURCES src/util/xor.c LIBS ${XU4_COMMON_LIBS})
  xu4_add_tool(xorbin SOURCES src/util/xorbin.c LIBS ${XU4_COMMON_LIBS})
endif()

install(TARGETS u4 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(XU4_BUILD_UTILS)
  install(TARGETS coord dumpsavegame recreate tlkconv u4dec u4enc u4unpackexe xor xorbin
          RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR})
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/armors.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/config.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/creatures.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/armors.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/config.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/creatures.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/graphics.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/maps.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/tileRules.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/tileset.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/weapons.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/xml.dtd
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/egaPalette.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/graphics.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/maps.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/music.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/sound.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/tileRules.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/tileset.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/vendorScript.xml
              ${CMAKE_CURRENT_SOURCE_DIR}/conf/weapons.xml
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/conf/dtd/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR}/dtd)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/u4.bmp ${CMAKE_CURRENT_SOURCE_DIR}/icons/u4.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/music/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR}/music
        FILES_MATCHING PATTERN "*.ogg")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sound/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR}/sound
        FILES_MATCHING PATTERN "*.ogg")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/graphics/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${XU4_INSTALL_LIBSUBDIR}/graphics)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/u4.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
