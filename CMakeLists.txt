cmake_minimum_required(VERSION 3.16)
project(ProductivityTracker VERSION 1.0.0 LANGUAGES CXX)

# Default to Release if no build type specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Platform-specific flags ---
if(MSVC)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- GLFW ---
# On Linux/macOS use find_package (system-installed). On Windows, FetchContent.
find_package(glfw3 CONFIG QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found, downloading via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# --- SQLite3 ---
# Linux/macOS: system package. Windows: download amalgamation.
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found, downloading amalgamation...")
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3460000.zip
    )
    FetchContent_MakeAvailable(sqlite3)
    add_library(SQLite::SQLite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(SQLite::SQLite3 PUBLIC ${sqlite3_SOURCE_DIR})
endif()

# --- Project sources ---
set(PROJECT_SOURCES
    src/main.cpp
    src/ui.cpp
    src/database_handler.cpp
    src/session.cpp
    src/timer.cpp
    src/timestamp.cpp
    src/query_parser.cpp
)

# --- Dear ImGui sources ---
set(IMGUI_SOURCES
    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_impl_glfw.cpp
    extern/imgui/imgui_impl_opengl3.cpp
)

add_executable(productivity_tracker ${PROJECT_SOURCES} ${IMGUI_SOURCES})

target_include_directories(productivity_tracker PRIVATE
    src
    extern/imgui
    extern/imgui/backends
    ${OPENGL_INCLUDE_DIR}
)

# --- Link libraries ---
if(TARGET glfw)
    target_link_libraries(productivity_tracker PRIVATE glfw)
else()
    target_link_libraries(productivity_tracker PRIVATE ${GLFW_LIBRARIES})
endif()

target_link_libraries(productivity_tracker PRIVATE SQLite3::SQLite3)

# Platform-specific linkage for OpenGL
if(APPLE)
    target_link_libraries(productivity_tracker PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    target_link_libraries(productivity_tracker PRIVATE opengl32)
else()
    target_link_libraries(productivity_tracker PRIVATE OpenGL::GL)
endif()

# --- NDEBUG on Release ---
target_compile_definitions(productivity_tracker PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)

# --- Ensure data folder exists in project root ---
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/data)

# --- Force the executable to run with project root as working directory ---
set_target_properties(productivity_tracker PROPERTIES
    # Windows: Visual Studio debugger uses this as cwd
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    # macOS: Xcode scheme working directory
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
