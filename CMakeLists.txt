cmake_minimum_required(VERSION 3.11.0)
project(GOESP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS_RELEASE "-O3 -s -fvisibility=hidden -flto -fwhole-program -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/GL/gl3w.c")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_dx9.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_win32.cpp") 

add_library(GOESP SHARED ${SOURCES})
target_link_libraries(GOESP SDL2 GL)
