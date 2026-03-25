cmake_minimum_required(VERSION 3.14)

project(TP3)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(
    GLOB
    source_files
    main.cpp
    src/*
)

file(
    GLOB
    arene_source_files
    arene.cpp
    src/*
)


set(externLibs
    z
    ncurses
    tinfo
)


add_definitions(-DNDEBUG)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -Wno-unused-result -O3 -g")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O0 -Wno-unused-result -g")


add_executable (
 Hex
 ${source_files}
)

target_link_libraries(Hex
    ${externLibs}
)



add_executable (
 Arene
 ${arene_source_files}
)

target_link_libraries(Arene
    ${externLibs}
)
