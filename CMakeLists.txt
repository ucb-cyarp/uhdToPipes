cmake_minimum_required(VERSION 3.5.1)
project(uhdToPipes C)

set(CMAKE_C_STANDARD 11)

#From https://github.com/microsoft/vscode-cmake-tools/issues/208
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/depends/gnuradio/cmake/Modules)

include_directories(src)

find_package(UHD)
include_directories(${UHD_INCLUDE_DIRS})

#From https://stackoverflow.com/questions/1620918/cmake-and-libpthread
find_package (Threads)

set(SRC_LIST
        src/main.c
        src/rxHandler.c
        src/rxHandler.h
        src/txHandler.c
        src/txHandler.h
        src/common.h)

add_executable(uhdToPipes ${SRC_LIST})
target_link_libraries(uhdToPipes ${UHD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})