# Source files to be used in the library
set(LIB_SOURCES
        src/aproflib.c
        )

set(LIB_INCLUDE
        ${PROJECT_SOURCE_DIR}/runtime/AProfLoopArrayListHooks/include
        )

add_library(AProfLoopArrayListHooks OBJECT
        ${LIB_SOURCES}
        )

target_include_directories(AProfLoopArrayListHooks
        PUBLIC ${LIB_INCLUDE}
        )


## TODO: this is for building LogMem.
############################################################
# Create an executable
############################################################

# Source fles for the binary
set(BINARY_SOURCES
        src/logmf.c
        )

# Add an executable with the above sources
add_executable(AprofLoopArrayListLogger ${BINARY_SOURCES})

# link the new hello_library target with the hello_binary target
target_link_libraries(AprofLoopArrayListLogger m rt)
