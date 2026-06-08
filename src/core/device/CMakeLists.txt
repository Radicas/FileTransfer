cmake_minimum_required(VERSION 3.16)

project(device VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(DEVICE_SOURCES
    devicemanager.cpp
)

set(DEVICE_HEADERS
    devicemanager.h
)

add_library(device STATIC
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_include_directories(device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(device PUBLIC
    Qt6::Core
    Qt6::Network
    network
    logger
)

setup_compiler_options(device)