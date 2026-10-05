cmake_minimum_required(VERSION 3.31)

project(Abomination
    VERSION 0.4.0
    DESCRIPTION "A retro first-person shooter inspired by Quake (1996)"
    LANGUAGES C CXX  # C: the generated GLAD loader is written in C
)

if(NOT MSVC)
    message(FATAL_ERROR "Abomination supports only MSVC (Visual Studio) on Windows.")
endif()

# --- Language standard -------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) # We do not use C++20 modules; skip the dependency scan

# --- Output folders ----------------------------------------------------------
# All executables (game, tests, later tools) go to one place: Build/<preset>/Binaries/<Config>/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)

# --- Project-wide helpers ----------------------------------------------------
include(CMake/CompilerOptions.cmake)

# --- Targets -----------------------------------------------------------------
add_subdirectory(ThirdParty/EnTT)
add_subdirectory(ThirdParty/GLAD)
add_subdirectory(SourceCode)

# --- Tests -------------------------------------------------------------------
enable_testing()
add_subdirectory(Tests)

# --- Packaging ---------------------------------------------------------------
include(CMake/Packaging.cmake)

# Make the game the default project to run in Visual Studio (F5).
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Abomination)
