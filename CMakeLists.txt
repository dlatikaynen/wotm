cmake_minimum_required(VERSION 4.3)
project(wotm)

set(CMAKE_CXX_STANDARD 23)

include_directories(C:/en/Emscripten/Release/include)

set(WOTM_SOURCES
	"src/main.cpp"
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

target_link_options(wotm PRIVATE
    "-sUSE_SDL=3"
    "-sUSE_SDL_TTF=3"
)

# Emscripten has no sdl3_mixer or sdl3_image port (only sdl3 + sdl3_ttf exist), so
# -sUSE_SDL_MIXER=3 / -sUSE_SDL_IMAGE=3 are silently ignored. Link our own wasm builds
# instead. They must come BEFORE the SDL=3 port on the link line so wasm-ld resolves
# MIX_*/IMG_* here and their SDL_* deps fall through to the port.
target_link_libraries(wotm PRIVATE
    C:/en/Emscripten/Release/lib/libSDL3_image.a
    C:/en/Emscripten/Release/lib/libSDL3_mixer.a)

if (EMSCRIPTEN)
    message(STATUS "Configuring Emscripten-specific flags")
	target_link_options(wotm PRIVATE
		"SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data")

	set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()