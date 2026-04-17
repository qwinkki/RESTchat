cmake_minimum_required(VERSION 3.16)
project(client)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)

add_executable(client src/main.cpp)

target_link_libraries(client PRIVATE CURL::libcurl)