# src/core CMakeLists.txt
# Core library and executable target for Horcrux

# Create the core library
add_library(horcrux_core STATIC
    build_node.cpp
    build_edge.cpp
    build_graph.cpp
)

# Set library properties
set_target_properties(horcrux_core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directory for the library
target_include_directories(horcrux_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler-specific options for library
target_compile_options(horcrux_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:
        -std=c++23
        -Wall -Wextra -Wpedantic
        -Werror=return-type
    >
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -std=c++23
        -Wall -Wextra -Wpedantic
        -Werror=return-type
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Create the horcrux executable
add_executable(horcrux
    main.cpp
)

# Set target properties
set_target_properties(horcrux PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME horcrux
)

# Link core library to executable
target_link_libraries(horcrux PRIVATE horcrux_core)

# Compiler-specific options
target_compile_options(horcrux PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:
        -std=c++23
        -Wall -Wextra -Wpedantic
        -Werror=return-type
    >
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -std=c++23
        -Wall -Wextra -Wpedantic
        -Werror=return-type
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Installation
install(TARGETS horcrux horcrux_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
