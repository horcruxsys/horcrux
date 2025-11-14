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

# Link with tl::expected for C++23 std::expected polyfill
target_link_libraries(horcrux_core PUBLIC tl::expected)

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

# Create example executable (optional)
option(HORCRUX_BUILD_EXAMPLES "Build example programs" ON)
if(HORCRUX_BUILD_EXAMPLES)
    add_executable(build_graph_example
        example.cpp
    )
    
    target_link_libraries(build_graph_example PRIVATE horcrux_core)
    
    target_compile_options(build_graph_example PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:
            -std=c++23
            -Wall -Wextra -Wpedantic
        >
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:
            -std=c++23
            -Wall -Wextra -Wpedantic
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /permissive-
        >
    )
    
    set_target_properties(build_graph_example PROPERTIES
        OUTPUT_NAME build_graph_example
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()

# Installation
install(TARGETS horcrux horcrux_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
