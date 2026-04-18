cmake_minimum_required(VERSION 4.3.1)

project(ProbeEngine VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)

add_executable(
    ENGINE
    src/main.c
    src/glad.c
)

target_include_directories(ENGINE PUBLIC
    include
)

target_link_libraries(ENGINE PUBLIC
    ${GLFW3_LIBRARY}
    OpenGL::GL
)