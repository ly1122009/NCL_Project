add_library(OSAL STATIC
    OSAL_Memory.c
    OSAL_Thread.c
)

target_include_directories(OSAL
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
