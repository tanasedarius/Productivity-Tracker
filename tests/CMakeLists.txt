# --- GoogleTest via FetchContent (no system install required) ---
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
# Prevent GoogleTest from overriding the parent project's compiler/linker settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# The core logic under test is header + source pairs compiled straight into
# the test binary. These have no GUI/OpenGL/SQLite dependencies, so the test
# target stays light and fast to build.
add_executable(core_tests
    test_core.cpp
    ${CMAKE_SOURCE_DIR}/src/timer.cpp
    ${CMAKE_SOURCE_DIR}/src/session.cpp
    ${CMAKE_SOURCE_DIR}/src/timestamp.cpp
    ${CMAKE_SOURCE_DIR}/src/query_parser.cpp
)

target_include_directories(core_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(core_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(core_tests)
