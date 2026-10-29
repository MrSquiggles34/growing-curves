cmake_minimum_required(VERSION 3.24)

project(growing-curves LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(dependencies/glfw-3.5.1)

# GLAD 1
add_library(glad STATIC
    dependencies/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    dependencies/glad/include
)

# OpenGL
find_package(OpenGL REQUIRED)

# Application
add_executable(growing-curves
    src/main.cpp
    src/directionField.cpp
    src/particle.cpp
    src/curveGrower.cpp
    src/distanceMap.cpp
    src/simulation.cpp
    src/occupancyGrid.cpp
)

# Include directories
target_include_directories(growing-curves PRIVATE
    dependencies
)

# Link libraries
target_link_libraries(growing-curves PRIVATE
    glfw
    glad
    OpenGL::GL
)

# Source directory for shaders
target_compile_definitions(growing-curves PRIVATE
    SIM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)