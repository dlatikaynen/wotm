cmake_minimum_required(VERSION 4.3)
project(wotm)

set(CMAKE_CXX_STANDARD 23)

# We use only plain headers (no import/export). Disable C++20 module scanning so
# Ninja doesn't drive builds through a dyndep graph whose stale header view can
# make transitively included declarations (like main.h `extern int screen;`)
# appear undefined after a header edit
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# cairo is prebuilt for wasm (image surface + freetype backend) and installed into
# C:/en/Emscripten/Release alongside pixman/freetype. See deps/build-cairo.cmd
include_directories(C:/en/Emscripten/Release/include)

set(WOTM_SOURCES
	"src/main.cpp"
    "src/render.cpp"
    "src/splash.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${WOTM_SOURCES})
add_executable(wotm ${WOTM_SOURCES})
set_target_properties(wotm PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(wotm PROPERTIES
    # This tells VS where the "root" of your source is for this project
    VS_PROJECT_IMPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_options(
    wotm PRIVATE
    "-sUSE_SDL=3"
    "-sUSE_SDL_TTF=3"
    "-sUSE_ZLIB=1"
)

# Emscripten has no sdl3_mixer or sdl3_image port (only sdl3 + sdl3_ttf exist), so
# -sUSE_SDL_MIXER=3 / -sUSE_SDL_IMAGE=3 are silently ignored. Link our own wasm builds
# instead. They must come BEFORE the SDL=3 port on the link line so wasm-ld resolves
# MIX_*/IMG_* here and their SDL_* deps fall through to the port.
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