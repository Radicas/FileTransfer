add_library(messagebus STATIC
    messagebus.cpp
    messagebus.h
    messagetypes.h
)

target_include_directories(messagebus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(messagebus
    Qt6::Core
    logger
)
