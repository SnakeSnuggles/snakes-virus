cmake_minimum_required(VERSION 3.10)
project(main_project)

set(CMAKE_CXX_STANDARD 17)

# Force static MSVC runtime
if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()

    # Set the _WIN32_WINNT version to target Windows 7 (0x0601)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Static GLFW build
set(BUILD_SHARED_LIBS OFF)

# Include directories
include_directories(
    libs/asio/include
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/um"  # Adjust this path if needed
)

# Gather all .cpp files in src/
file(GLOB SRC_FILES src/*.cpp)

# Define the executable
add_executable(main ${SRC_FILES})

# Add preprocessor defines
target_compile_definitions(main PRIVATE
    ASIO_STANDALONE
)
