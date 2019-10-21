cmake_minimum_required(VERSION 3.5.1)
project(uhdToPipes C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH depends/gnuradio/cmake)

include_directories(src)

find_package(UHD)
include_directories(${UHD_INCLUDE_DIRS})

set(SRC_LIST
        src/main.c
        src/rxHandler.c
        src/rxHandler.h
        src/txHandler.c
        src/txHandler.h
        src/common.h)

add_executable(uhdToPipes ${SRC_LIST})
target_link_libraries(uhdToPipes ${UHD_LIBRARIES})