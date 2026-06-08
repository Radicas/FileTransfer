cmake_minimum_required(VERSION 3.16)

project(app VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(APP_SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SRC_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    mainwindow
    widgets
    device
    filesystem
    network
    logger
    theme
    json
    common_utils
)

setup_compiler_options(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if(WIN32)
    install(FILES
        $<TARGET_FILE:Qt6::Core>
        $<TARGET_FILE:Qt6::Gui>
        $<TARGET_FILE:Qt6::Widgets>
        $<TARGET_FILE:Qt6::Sql>
        $<TARGET_FILE:Qt6::Network>
        $<TARGET_FILE:Qt6::Concurrent>
        DESTINATION bin
        OPTIONAL
    )
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()