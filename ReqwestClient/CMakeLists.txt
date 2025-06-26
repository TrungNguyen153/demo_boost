
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/include/*.h")

add_executable(ReqwestClient ${SOURCES} ${HEADERS})

target_include_directories(ReqwestClient PRIVATE
    "vendor"
    "src/include"
    "C:/Program Files/OpenSSL-Win64/include")

target_link_directories(
    ReqwestClient
    PRIVATE
    "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MT")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode.")
    if (MSVC)
        message(STATUS "MSVC Debug mode: Set /bigobj for boost")
        target_compile_options(ReqwestClient PRIVATE /bigobj)
    endif()
    # Add your Debug-specific actions here
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode.")
    # Add your Release-specific actions here
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Building in Release with Debug Info mode.")
    # Add your RelWithDebInfo-specific actions here
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message(STATUS "Building in Minimum Size Release mode.")
    # Add your MinSizeRel-specific actions here
else()
    message(STATUS "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

# Crypt32.lib (windows)
# libcrypto + libssl (openssl)
target_link_libraries(ReqwestClient PRIVATE
    Crypt32.lib
    libcrypto_static.lib
    libssl_static.lib)
