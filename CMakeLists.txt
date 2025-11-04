cmake_minimum_required(VERSION 3.16)

project(Socket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#* Settings for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)