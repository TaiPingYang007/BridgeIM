cmake_minimum_required(VERSION 3.16)
project(BridgeIM CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -g)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ProtobufTargets.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ServerModels.cmake)

add_subdirectory(src)

