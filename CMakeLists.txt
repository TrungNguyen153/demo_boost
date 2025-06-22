
cmake_minimum_required(VERSION 3.10)

project(BoostExplore VERSION 1.0 LANGUAGES CXX)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# --- Display the detected C++ compiler ---
message(STATUS "Current CXX Compiler: ${CMAKE_CXX_COMPILER}")
# --- You can also display the compiler ID and version ---
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

add_subdirectory(ReqwestClient)
