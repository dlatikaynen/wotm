cmake_minimum_required(VERSION 4.3)
project(platformer)

set(CMAKE_CXX_STANDARD 23)

include_directories(C:/en/Emscripten/Release/include)

set(PLATFORMER_SOURCES
	"main.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PLATFORMER_SOURCES})

add_executable(platformer ${PLATFORMER_SOURCES})

set_target_properties(platformer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(platformer PROPERTIES
    # This tells VS where the "root" of your source is for this project
    VS_PROJECT_IMPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_options(platformer PRIVATE 
    "-sUSE_SDL=3" 
    "-sUSE_SDL_IMAGE=3"
    "-sUSE_SDL_MIXER=3"
    "-sUSE_SDL_TTF=3"
)

if (EMSCRIPTEN)
    message(STATUS "Configuring Emscripten-specific flags")
	target_link_options(platformer PRIVATE
		"SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data")

	set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()