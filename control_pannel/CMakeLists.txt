cmake_minimum_required(VERSION 3.10)
project(control_panel)

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
endif()

# Static GLFW build
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(libs/glfw)

# GLAD (OpenGL loader)
add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui source files
set(IMGUI_SRC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Include directories
include_directories(
    libs/imgui
    libs/imgui/backends
    libs/glfw/include
    libs/glad/include
    libs/asio/include
)

# Gather all .cpp files in src/
file(GLOB SRC_FILES src/*.cpp)

# Define the executable
add_executable(control_panel ${SRC_FILES} ${IMGUI_SRC})

# Add preprocessor defines
target_compile_definitions(control_panel PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    ASIO_STANDALONE
)

# Link libraries
target_link_libraries(control_panel
    glfw
    glad
    OpenGL::GL
)
