cmake_minimum_required(VERSION 3.5)

project(chordagon)

find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(libs/glfw)
add_subdirectory(libs/libremidi)

include_directories(
    libs/glad/include
    libs/libremidi/include
    libs/readerwriterqueue
    libs/MTS-ESP/Client
    libs/stb
)
set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h")

add_executable(${PROJECT_NAME}
    src/main.cpp
    libs/glad/src/glad.c
    libs/MTS-ESP/Client/libMTSClient.cpp
)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} glfw libremidi)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
)
