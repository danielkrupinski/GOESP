set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
  URL_HASH SHA256=24564E3B712D3EB30AC9A85D92F7D720F60CC0173730AC166F27DDA7FED76CB2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(Tests MemoryTests.cpp)
target_link_libraries(Tests gtest_main ${CMAKE_DL_LIBS} Threads::Threads)

include(GoogleTest)
gtest_discover_tests(Tests)
