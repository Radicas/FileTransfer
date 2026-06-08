cmake_minimum_required(VERSION 3.16)

project(widgets VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(WIDGETS_SOURCES
    devicetreeview.cpp
    filebrowserpanel.cpp
    transferprogresspanel.cpp
)

set(WIDGETS_HEADERS
    devicetreeview.h
    filebrowserpanel.h
    transferprogresspanel.h
)

add_library(widgets STATIC
    ${WIDGETS_SOURCES}
    ${WIDGETS_HEADERS}
)

target_include_directories(widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(widgets PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    device
    filesystem
    network
    logger
)

setup_compiler_options(widgets)