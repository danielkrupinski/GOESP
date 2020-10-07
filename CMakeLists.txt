cmake_minimum_required(VERSION 3.11.0)
project(GOESP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS_RELEASE "-O3 -s -fvisibility=hidden -flto -fwhole-program -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} --no-gnu-unique -fno-rtti")

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/GOESP/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/GL/gl3w.c")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_dx9.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_win32.cpp") 

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_library(GOESP SHARED ${SOURCES})

if (APPLE)
    target_link_options(GOESP INTERFACE "-framework OpenGL")
    target_link_libraries(GOESP SDL2)
else()
    target_link_libraries(GOESP SDL2 GL)
endif()
