cmake_minimum_required(VERSION 3.16)

project(FileTransfer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Sql 
    Network 
    Concurrent
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "C++DesktopAppTemplate")
set(CPACK_PACKAGE_VENDOR "Template Team")
set(CPACK_PACKAGE_CONTACT "support@example.com")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CppAppTemplate")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-Windows")
endif()

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-macOS")
endif()

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-Linux")
endif()

include(CPack)

set(PROJECT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${PROJECT_SRC_DIR})

function(setup_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4)
        target_compile_definitions(${target} PRIVATE 
            _UNICODE 
            UNICODE 
            NOMINMAX
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            set_target_properties(${target} PROPERTIES
                WIN32_EXECUTABLE TRUE
            )
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE 
            -Wall 
            -Wextra 
            -Wpedantic
            $<$<CONFIG:Release>:-O2>
            $<$<CONFIG:Debug>:-g>
        )
        if(WIN32)
            target_compile_definitions(${target} PRIVATE 
                _UNICODE 
                UNICODE 
                NOMINMAX
            )
        endif()
    endif()
endfunction()

add_subdirectory(src)
