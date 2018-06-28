cmake_minimum_required (VERSION 3.8)
set( CMAKE_VERBOSE_MAKEFILE ON )

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (limonada)

#properly disable insource building, as CMAKE_DISABLE_IN_SOURCE_BUILD never works for me
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(SEND_ERROR "Insource building is disabled.")
endif()

set(SOURCES
arg.h
buffer.c
buffer.h
dirent.h
gui.c
gui.h
main.c
platform.c
platform.h
slice.c
slice.h
state.c
state.h
stb_image.h
stb_image.c
)

add_executable(limonada ${SOURCES})

target_link_libraries(limonada -lSDL2 -lSDL2_main -lSDL2_image -lSDL2_mixer -lSDL2_ttf)
