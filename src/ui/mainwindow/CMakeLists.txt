cmake_minimum_required(VERSION 3.16)

project(mainwindow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(MAINWINDOW_SOURCES
    mainwindow.cpp
)

set(MAINWINDOW_HEADERS
    mainwindow.h
)

add_library(mainwindow STATIC
    ${MAINWINDOW_SOURCES}
    ${MAINWINDOW_HEADERS}
)

target_include_directories(mainwindow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(mainwindow PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    widgets
    device
    filesystem
    network
    logger
    theme
)

setup_compiler_options(mainwindow)