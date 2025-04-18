cmake_minimum_required(VERSION 3.10)
project(control)

set(CMAKE_CXX_STANDARD 17)

# Static runtime on MSVC
if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

# Static GLFW
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(libs/glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui sources
set(IMGUI_SRC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

include_directories(
    libs/imgui
    libs/imgui/backends
    libs/glfw/include
    libs/asio/include
)

add_executable(control main.cpp ${IMGUI_SRC})

target_link_libraries(control glfw OpenGL::GL)