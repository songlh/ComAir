# Source files to be used in the library
set(LIB_SOURCES
        src/aproflib.c
        )

set(LIB_INCLUDE
        ${PROJECT_SOURCE_DIR}/runtime/AProfCounterHooks/include
        )

add_library(AProfCounterHooks STATIC
        ${LIB_SOURCES}
        )

target_include_directories(AProfCounterHooks
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
add_executable(AprofCounterLogger ${BINARY_SOURCES})

# link the new hello_library target with the hello_binary target
target_link_libraries(AprofCounterLogger m rt)
