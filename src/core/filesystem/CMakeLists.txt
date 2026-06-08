cmake_minimum_required(VERSION 3.16)

project(filesystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(FILESYSTEM_SOURCES
    filesystemmodel.cpp
)

set(FILESYSTEM_HEADERS
    filesystemmodel.h
)

add_library(filesystem STATIC
    ${FILESYSTEM_SOURCES}
    ${FILESYSTEM_HEADERS}
)

target_include_directories(filesystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(filesystem PUBLIC
    Qt6::Core
    Qt6::Widgets
    network
    logger
)

setup_compiler_options(filesystem)