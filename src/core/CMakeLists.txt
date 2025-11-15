# src/core CMakeLists.txt
# Core library and executable target for Horcrux

# Create the core library
add_library(horcrux_core STATIC
    build_node.cpp
    build_edge.cpp
    build_graph.cpp
    local_cache.cpp
    simple_builder.cpp
    android_toolchain.cpp
    android_sandbox.cpp
    android_ndk_compiler.cpp
    android_java_compiler.cpp
    android_kotlin_compiler.cpp
    android_compose_compiler.cpp
    android_dex_compiler.cpp
    android_resources.cpp
    android_manifest_merger.cpp
    android_apk_packager.cpp
    android_aab_packager.cpp
    android_variant.cpp
    gradle_parser.cpp
    workspace_config_generator.cpp
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

# Link with tl::expected for C++23 std::expected polyfill and tinyxml2 for XML parsing
target_link_libraries(horcrux_core PUBLIC tl::expected OpenSSL::Crypto tinyxml2)

# Library-specific optimizations (global flags already applied)
# No additional flags needed - using global settings

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

# Executable-specific settings (global flags already applied)
# No additional flags needed - using global settings

# Create example executable (optional)
option(HORCRUX_BUILD_EXAMPLES "Build example programs" ON)
if(HORCRUX_BUILD_EXAMPLES)
    add_executable(build_graph_example
        example.cpp
    )
    
    target_link_libraries(build_graph_example PRIVATE horcrux_core)
    
    # Example-specific settings (global flags already applied)
    # No additional flags needed - using global settings
    
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
