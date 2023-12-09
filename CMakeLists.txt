cmake_minimum_required(VERSION 3.27)
project(cwebserver C)

set(CMAKE_C_STANDARD 23)

add_library(cwebserver STATIC cwebserver.c)
target_link_libraries(cwebserver PRIVATE ws2_32)
