cmake_minimum_required(VERSION 3.16)

project(logger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

add_library(logger STATIC
    logger.h
    logger.cpp
)

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(logger PUBLIC
    Qt6::Core
)

setup_compiler_options(logger)