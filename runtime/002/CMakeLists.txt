# Source files to be used in the library
set(LIB_SOURCES
        src/logger.c
        src/aproflib.c
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


## TODO: this is for building LogMem.
############################################################
# Create an executable
############################################################

# Source fles for the binary
set(BINARY_SOURCES
        src/log_mem_to_file.c
        )

# Add an executable with the above sources
add_executable(AprofLogMem ${BINARY_SOURCES})

# link the new hello_library target with the hello_binary target
target_link_libraries(AprofLogMem rt)


## TODO: this is for testing runtime lib.
############################################################
# Create an executable
############################################################

# Source fles for the binary
set(BINARY_SOURCES
        src/test_runtime.c
        )

# Add an executable with the above sources
add_executable(testHook ${BINARY_SOURCES})

# link the new hello_library target with the hello_binary target
target_link_libraries(testHook
        PRIVATE AProfHooks
        )