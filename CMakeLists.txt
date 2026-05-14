cmake_minimum_required(VERSION 3.16)
project(dx_ball LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dx_ball main.cpp)
add_executable(dx_ball_part1 individual_parts/main_part1.cpp)
add_executable(dx_ball_part2 individual_parts/main_part2.cpp)
add_executable(dx_ball_part3 individual_parts/main_part3.cpp)
add_executable(dx_ball_part4 individual_parts/main_part4.cpp)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

target_link_libraries(dx_ball PRIVATE OpenGL::GL GLUT::GLUT)
target_link_libraries(dx_ball_part1 PRIVATE OpenGL::GL GLUT::GLUT)
target_link_libraries(dx_ball_part2 PRIVATE OpenGL::GL GLUT::GLUT)
target_link_libraries(dx_ball_part3 PRIVATE OpenGL::GL GLUT::GLUT)
target_link_libraries(dx_ball_part4 PRIVATE OpenGL::GL GLUT::GLUT)

if(WIN32)
    target_link_libraries(dx_ball PRIVATE winmm)
    target_link_libraries(dx_ball_part1 PRIVATE winmm)
    target_link_libraries(dx_ball_part2 PRIVATE winmm)
    target_link_libraries(dx_ball_part3 PRIVATE winmm)
    target_link_libraries(dx_ball_part4 PRIVATE winmm)
endif()
