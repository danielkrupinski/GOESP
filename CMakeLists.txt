cmake_minimum_required(VERSION 3.13.0)
project(GOESP)

set(CMAKE_C_FLAGS_RELEASE "-O3 -march=core2 -mtune=native -fvisibility=hidden -flto -fno-exceptions -DNDEBUG")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-rtti")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} --no-gnu-unique")
endif()

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/GOESP/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/GL/gl3w.c")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_dx9.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_win32.cpp") 

include_directories(/usr/local/include/ /usr/include/SDL2/)
link_directories(/usr/local/lib)

add_library(GOESP SHARED ${SOURCES})

if (APPLE)
    target_link_options(GOESP INTERFACE "-framework OpenGL")
    target_link_libraries(GOESP SDL2)
else()
    target_link_options(GOESP PRIVATE LINKER:--no-undefined)
    target_link_libraries(GOESP SDL2 GL dl)
endif()
