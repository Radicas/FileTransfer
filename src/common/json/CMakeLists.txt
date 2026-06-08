cmake_minimum_required(VERSION 3.16)

project(json VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

add_library(json STATIC
    jsonutil.h
    jsonutil.cpp
)

target_include_directories(json PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(json PUBLIC
    Qt6::Core
    logger
)

if(MSVC)
    target_compile_options(json PRIVATE
        /bigobj
        /Zm200
    )
endif()

setup_compiler_options(json)