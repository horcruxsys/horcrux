# src/core CMakeLists.txt
# Core executable target for Horcrux

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
install(TARGETS horcrux
    RUNTIME DESTINATION bin
)
