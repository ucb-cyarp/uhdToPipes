cmake_minimum_required(VERSION 3.15)
project(uhdToPipes C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH depends/gnuradio/cmake)

include_directories(src)

find_package(UHD)
include_directories(${UHD_INCLUDE_DIRS})

add_executable(uhdToPipes src/main.c)
target_link_libraries(uhdToPipes ${UHD_LIBRARIES})