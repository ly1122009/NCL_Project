# Add the library
add_library(Library STATIC
    src/Math.cpp
)

target_include_directories(Library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(Library
    PUBLIC
        OSAL
        Interface
)
