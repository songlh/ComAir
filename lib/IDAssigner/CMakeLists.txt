add_library(IDAssignerPass MODULE
    # List your source files here.
    IDAssigner.cpp
    IDTagger.cpp
    IDManager.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(IDAssignerPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(IDAssignerPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)


add_library(ID STATIC
    # List your source files here.
    IDAssigner.cpp
    IDTagger.cpp
    IDManager.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(ID PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(ID PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)