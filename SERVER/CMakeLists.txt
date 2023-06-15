cmake_minimum_required(VERSION 3.0)

project(CFTPChat-Server)

set(SOURCE_FILES main.c)

add_library(fonts STATIC ${SOURCE_FILES})

add_executable(main ${SOURCE_FILES} main.o)

target_link_libraries(main fonts)

target_link_libraries(main Ws2_32)