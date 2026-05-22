cmake_minimum_required(VERSION 4.2)

project(spine_example)
file(GLOB spine_c "src/spine/*.c")

add_executable(${PROJECT_NAME} main.c spine_impl.c ${spine_c} third_party/glad/src/glad.c)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    third_party
    third_party/glad/include
    third_party/glfw/include
)
add_subdirectory(third_party/glfw)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    opengl32
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
