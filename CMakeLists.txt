cmake_minimum_required(VERSION 3.13)

project(codecrafters-http-server LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCE_FILES src/*.c)
add_executable(http-server ${SOURCE_FILES})
target_include_directories(http-server PRIVATE src)
