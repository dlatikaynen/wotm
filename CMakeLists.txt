cmake_minimum_required(VERSION 4.3)
project(wotm)

set(CMAKE_CXX_STANDARD 23)

# only plain headers (no import/export). disable C++20 module scanning so
# Ninja doesn't drive builds through a dyndep graph whose stale header view can
# make transitively included declarations (like main.h `extern int screen;`)
# appear undefined after a header change
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# cairo is prebuilt for wasm (image surface + freetype backend) and installed into
# C:/en/Emscripten/Release alongside pixman/freetype. See deps/build-cairo.cmd
# the cairo/ and freetype2/ subdirs hold the actual headers (cairo.h, cairo-ft.h,
# ft2build.h + FT_FREETYPE_H), so they go on the include path directly, this is
# what cairo's pkg-config would have emitted (-I.../cairo -I.../freetype2)
include_directories(
    C:/en/Emscripten/Release/include
    C:/en/Emscripten/Release/include/cairo
    C:/en/Emscripten/Release/include/freetype2
)

# scripting: Lua 5.4 + sol2
include(FetchContent)
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        develop
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  no-cmake-here
)
FetchContent_MakeAvailable(lua sol2)

file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
# lua.c (interpreter) and luac.c (compiler) each have their own main()
list(REMOVE_ITEM LUA_SOURCES
    "${lua_SOURCE_DIR}/src/lua.c"
    "${lua_SOURCE_DIR}/src/luac.c")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "${lua_SOURCE_DIR}/src")

# glob all .cpp under src/ so new files are picked up without editing this list
# CONFIGURE_DEPENDS makes the build re-run the glob when files are added/removed
file(GLOB_RECURSE WOTM_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${WOTM_SOURCES})
add_executable(wotm ${WOTM_SOURCES})

# sol2 needs C++ exception catching, which Emscripten turns off by default.
target_compile_options(wotm PRIVATE -fexceptions)
target_link_options(wotm PRIVATE -fexceptions)
target_include_directories(wotm PRIVATE "${sol2_SOURCE_DIR}/include")
target_link_libraries(wotm PRIVATE lua)
set_target_properties(wotm PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(wotm PROPERTIES
    # this tells VS where the root is
    VS_PROJECT_IMPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_options(
    wotm PRIVATE
    "-sUSE_SDL=3"
    "-sUSE_SDL_TTF=3"
    "-sUSE_ZLIB=1"
)

# emscripten has no sdl3_mixer or sdl3_image port (only sdl3 + sdl3_ttf exist), so
# -sUSE_SDL_MIXER=3 / -sUSE_SDL_IMAGE=3 are silently ignored. our own wasm builds
# must come before the SDL=3 port on the link line so wasm-ld resolves
# MIX_*/IMG_* here and their SDL* deps fall through to the port.
target_link_libraries(
    wotm PRIVATE
    C:/en/Emscripten/Release/lib/libSDL3_image.a
    C:/en/Emscripten/Release/lib/libSDL3_mixer.a
    # cairo must precede pixman/freetype
    C:/en/Emscripten/Release/lib/libcairo.a
    C:/en/Emscripten/Release/lib/libpixman-1.a
    C:/en/Emscripten/Release/lib/libfreetype.a
)

if (EMSCRIPTEN)
    message(STATUS "Configuring Emscripten-specific flags")
	target_link_options(wotm PRIVATE
		"SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data")

	set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()