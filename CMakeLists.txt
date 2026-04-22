cmake_minimum_required(VERSION 3.16)
project(dx_ball LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dx_ball main.cpp)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

target_link_libraries(dx_ball PRIVATE OpenGL::GL GLUT::GLUT)

if(WIN32)
    target_link_libraries(dx_ball PRIVATE winmm)
endif()
