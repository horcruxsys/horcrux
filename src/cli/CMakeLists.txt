# src/cli CMakeLists.txt
# CLI interface for Horcrux

# Create the CLI library
add_library(horcrux_cli STATIC
    target_parser.cpp
    build_executor.cpp
)

# Set library properties
set_target_properties(horcrux_cli PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(horcrux_cli PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link with core library and dependencies
target_link_libraries(horcrux_cli PUBLIC
    horcrux_core
    tl::expected
)

# Create the CLI executable
add_executable(horcrux_cli_exe
    main.cpp
)

# Set target properties
set_target_properties(horcrux_cli_exe PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME horcrux-cli
)

# Link libraries
target_link_libraries(horcrux_cli_exe PRIVATE
    horcrux_cli
    horcrux_core
)

# Installation
install(TARGETS horcrux_cli_exe horcrux_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
