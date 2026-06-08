set(CMAKE_AUTOMOC ON)

add_library(common_utils STATIC
    VersionManager.cpp
    versionmanager.h
    firewallmanager.cpp
    firewallmanager.h
)

target_include_directories(common_utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SRC_DIR}
)

target_link_libraries(common_utils
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    logger
)

setup_compiler_options(common_utils)