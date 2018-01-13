# Source files to be used in the library
set(LIB_SOURCES
        src/AProfHooks.c
        src/hashMap.c
        src/inputPro.c
        )

set(LIB_INCLUDE
        ${PROJECT_SOURCE_DIR}/runtime/AProfHooks/include
        )

add_library(AProfHooks STATIC
        ${LIB_SOURCES}
        )

target_include_directories(AProfHooks
        PUBLIC ${LIB_INCLUDE}
        )
