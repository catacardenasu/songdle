cmake_minimum_required(VERSION 3.28)
project(Songdle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put the .exe files in one place inside the build folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- SFML 3: downloaded and built automatically the first time ----
include(FetchContent)
FetchContent_Declare(SFML
        URL https://github.com/SFML/SFML/archive/refs/tags/3.0.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        EXCLUDE_FROM_ALL
        SYSTEM)
set(BUILD_SHARED_LIBS OFF)        # link SFML statically: no DLLs to copy
set(SFML_BUILD_AUDIO OFF)         # we only need graphics + window + system
set(SFML_BUILD_NETWORK OFF)
FetchContent_MakeAvailable(SFML)

# ---- Code shared by the game and the tests ----
add_library(songdle_core STATIC
        src/DataLoader.cpp
        src/Game.cpp
        src/Benchmark.cpp)
target_include_directories(songdle_core PUBLIC src)

# ---- The game (SFML window) ----
add_executable(songdle src/main.cpp src/GameUI.cpp)
target_link_libraries(songdle PRIVATE songdle_core SFML::Graphics)

# ---- The correctness test (console, no SFML) ----
add_executable(test_same_results tests/test_same_results.cpp)
target_link_libraries(test_same_results PRIVATE songdle_core)