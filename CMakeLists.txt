cmake_minimum_required(VERSION 3.13.0)
project(GOESP)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (NOT WIN32)
    set(CMAKE_C_FLAGS_RELEASE "-O3 -march=core2 -mtune=native -flto -fno-exceptions -DNDEBUG")
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()

set(CMAKE_CXX_STANDARD 20)

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-rtti")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} --no-gnu-unique")
endif()

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/GOESP/*.cpp")

if (NOT WIN32)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/GL/gl3w.c")
    list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_dx9.cpp") 
    list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_win32.cpp") 
else()
    list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_opengl3.cpp") 
    list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/GOESP/imgui/imgui_impl_sdl.cpp") 
endif()

# FreeType sources
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/autofit/autofit.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/cff/cff.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftbase.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftbitmap.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftdebug.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftglyph.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftinit.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftsynth.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/base/ftsystem.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/psaux/psaux.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/pshinter/pshinter.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/psnames/psnames.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/raster/raster.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/sfnt/sfnt.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/smooth/smooth.c")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/GOESP/freetype/src/truetype/truetype.c")

add_compile_definitions(FT2_BUILD_LIBRARY)

if (NOT WIN32)
    include_directories(/usr/local/include/ /usr/include/SDL2/ "${CMAKE_SOURCE_DIR}/GOESP/freetype/")
    link_directories(/usr/local/lib)
else()
    include_directories("${CMAKE_SOURCE_DIR}/GOESP/freetype/")
endif()

add_library(GOESP SHARED ${SOURCES})

if (APPLE)
    target_link_options(GOESP INTERFACE "-framework OpenGL")
    target_link_libraries(GOESP SDL2)
elseif (NOT WIN32)
    target_link_options(GOESP PRIVATE LINKER:--no-undefined)
    target_link_libraries(GOESP SDL2 GL dl)
endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()
