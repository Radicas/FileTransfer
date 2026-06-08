cmake_minimum_required(VERSION 3.16)

project(theme VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(theme STATIC
    thememanager.cpp
    thememanager.h
    theme.qrc
)

target_include_directories(theme PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(theme PUBLIC
    Qt6::Core
    Qt6::Widgets
    logger
)

setup_compiler_options(theme)