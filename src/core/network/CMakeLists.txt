cmake_minimum_required(VERSION 3.16)

project(network VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(NETWORK_SOURCES
    devicediscoverer.cpp
    filetransferservice.cpp
)

set(NETWORK_HEADERS
    devicediscoverer.h
    filetransferservice.h
    networkprotocol.h
)

add_library(network STATIC
    ${NETWORK_SOURCES}
    ${NETWORK_HEADERS}
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(network PUBLIC
    Qt6::Core
    Qt6::Network
    logger
)

setup_compiler_options(network)