cmake_minimum_required(VERSION 3.16)
project(dx_ball_parts LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Try to find GLUT, but continue if not found
find_package(GLUT)
if(NOT GLUT_FOUND)
    # Fallback: assume GLUT is available via system paths
    set(GLUT_LIBRARIES freeglut)
    set(GLUT_INCLUDE_DIRS "")
    message(STATUS "GLUT not found via find_package, using system defaults")
endif()

add_executable(dx_ball_part1 main_part1.cpp)
add_executable(dx_ball_part2 main_part2.cpp)
add_executable(dx_ball_part3 main_part3.cpp)
add_executable(dx_ball_part4 main_part4.cpp)

target_link_libraries(dx_ball_part1 PRIVATE OpenGL::GL ${GLUT_LIBRARIES} glu32)
target_link_libraries(dx_ball_part2 PRIVATE OpenGL::GL ${GLUT_LIBRARIES} glu32)
target_link_libraries(dx_ball_part3 PRIVATE OpenGL::GL ${GLUT_LIBRARIES} glu32)
target_link_libraries(dx_ball_part4 PRIVATE OpenGL::GL ${GLUT_LIBRARIES} glu32)

if(WIN32)
    target_link_libraries(dx_ball_part1 PRIVATE winmm)
    target_link_libraries(dx_ball_part2 PRIVATE winmm)
    target_link_libraries(dx_ball_part3 PRIVATE winmm)
    target_link_libraries(dx_ball_part4 PRIVATE winmm)
endif()
