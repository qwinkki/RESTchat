cmake_minimum_required(VERSION 3.16)
project(server)

set(CMAKE_CXX_STANDARD 17)

find_package(PostgreSQL REQUIRED)

add_executable(server src/main.cpp)

target_include_directories(server PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(server PRIVATE pqxx ${PostgreSQL_INCLUDE_DIRS})