
include_guard(GLOBAL)

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  IZ_MSVC_VERSION
  IZ_MSVC_EDITION
  IZ_MSVC_TOOLSET)

message(STATUS "Toolchain MSVC")

if (NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
    if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
        set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
    if (NOT DEFINED CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE)
        set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x86")
        if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")
            set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")
        elseif (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "arm64")
        endif()
    endif()
    if (NOT DEFINED CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME "x86")
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
            set(CMAKE_VS_PLATFORM_NAME "x64")
        elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(CMAKE_VS_PLATFORM_NAME "arm64")
        endif()
    endif()
endif()


# Find VSWHERE for search version visual studio installed
block(SCOPE_FOR VARIABLES)
    cmake_path(CONVERT
               "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
               TO_CMAKE_PATH_LIST vswhere.dir1 NORMALIZE)

    cmake_path(CONVERT
               "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
               TO_CMAKE_PATH_LIST vswhere.dir2 NORMALIZE)

    # This only temporarily affects the variable since we're inside a block.
    list(APPEND CMAKE_SYSTEM_PROGRAM_PATH "${vswhere.dir1}" "${vswhere.dir2}")
    find_program(VSWHERE_EXECUTABLE names vswhere DOC "Visual Studio Locator"
                 REQUIRED)
    message(STATUS "Found VSWHERE at ${VSWHERE_EXECUTABLE}")
endblock()


# Visual Studio Installation Search
if (DEFINED IZ_MSVC_EDITION)
    set(product "Microsoft.VisualStudio.Product.${IZ_MSVC_EDITION}")
else()
    set(product "*")
endif()
message(CHECK_START "Searching for Visual Studio ${IZ_MSVC_EDITION}")
execute_process(COMMAND "${VSWHERE_EXECUTABLE}" -nologo -nocolor
      -format json
      -products "${product}"
      -utf8
      -sort
    ENCODING UTF-8
    OUTPUT_VARIABLE candidates
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(JSON candidates.length LENGTH "${candidates}")
string(JOIN " " error "Could not find Visual Studio"
  "${IZ_MSVC_VERSION}"
  "${IZ_MSVC_EDITION}")
if (candidates.length EQUAL 0)
    message(CHECK_FAIL "no products")
    # You can choose to either hard fail here, or continue
    message(FATAL_ERROR "${error}")
endif()

if (NOT IZ_MSVC_VERSION)
    string(JSON candidate.install.path GET "${candidates}" 0 "installationPath")
else()
    # Unfortunately, range operations are inclusive in CMake for god knows why
    math(EXPR stop "${candidates.length} - 1")
    foreach (idx RANGE 0 ${stop})
        string(JSON version GET "${candidates}" ${idx} "catalog"
             "productLineVersion")
        if (version VERSION_EQUAL IZ_MSVC_VERSION)
            string(JSON candidate.install.path
          GET "${candidates}" ${idx} "installationPath")
            break()
        endif()
    endforeach()
endif()
if (NOT candidate.install.path)
    message(CHECK_FAIL "no install path found")
    message(FATAL_ERROR "${error}")
endif()
cmake_path(
    CONVERT "${candidate.install.path}"
    TO_CMAKE_PATH_LIST candidate.install.path
    NORMALIZE)
message(CHECK_PASS
          "found : candidate.install.path = ${candidate.install.path}")
set(IZ_MSVC_INSTALL_PATH "${candidate.install.path}"
    CACHE PATH "Visual Studio Installation Path")

# Windows SDK Root Discovery
message(CHECK_START "Searching for Windows SDK Root Directory")
cmake_host_system_information(RESULT IZ_MSVC_WINDOWS_SDK_ROOT QUERY
      WINDOWS_REGISTRY "HKLM/SOFTWARE/Microsoft/Windows Kits/Installed Roots"
      VALUE "KitsRoot10"
      VIEW BOTH
      ERROR_VARIABLE error
      )
if (error)
    message(CHECK_FAIL "not found : ${error}")
else()
    cmake_path(CONVERT "${IZ_MSVC_WINDOWS_SDK_ROOT}"
      TO_CMAKE_PATH_LIST IZ_MSVC_WINDOWS_SDK_ROOT
      NORMALIZE)
    message(CHECK_PASS
            "found : IZ_MSVC_WINDOWS_SDK_ROOT ${IZ_MSVC_WINDOWS_SDK_ROOT}")
endif()


function (msvc::directory out-var)
    if (${out-var})
        return()
    endif()
    cmake_parse_arguments(ARG "" "VARIABLE;PATH;DOC" "" ${ARGN})
    message(CHECK_START "Searching for ${ARG_DOC}")
    # We want to get the list of options, but *not* the full path string, hence
    # the use of `RELATIVE`
    message(STATUS "Finding candidates relative at ${ARG_PATH}")
    file(GLOB candidates
    LIST_DIRECTORIES YES
    RELATIVE "${ARG_PATH}"
    "${ARG_PATH}/*")
    list(SORT candidates COMPARE NATURAL ORDER DESCENDING)

    if (NOT DEFINED ARG_VARIABLE OR "${${ARG_VARIABLE}}" STREQUAL "")
        message(STATUS "NOT SET ARG_VARIABLE => use default")
        list(GET candidates 0 ${out-var})
    else()
        foreach (candidate IN LISTS candidates)
            message(STATUS
                    "${out-var} - ${${ARG_VARIABLE}} with Candide ${candidate}")
            if ("${ARG_VARIABLE}" VERSION_EQUAL candidate)
                set(${out-var} "${candidate}")
                break()
            endif()
        endforeach()
    endif()
    if (NOT ${out-var})
        message(CHECK_FAIL "not found")
    else()
        message(CHECK_PASS "found : ${out-var} = ${${out-var}}")
        set(${out-var} "${${out-var}}" CACHE INTERNAL "${out-var} value")
    endif()
endfunction()


cmake_language(CALL msvc::directory
  IZ_MSVC_TOOLS_VERSION
  VARIABLE IZ_MSVC_TOOLSET
  PATH "${IZ_MSVC_INSTALL_PATH}/VC/Tools/MSVC"
  DOC "MSVC Toolset")

# Your CMAKE_SYSTEM_VERSION should line up with the minimum SDK version you're
# targeting exactly.
cmake_language(CALL msvc::directory
    IZ_MSVC_WINDOWS_SDK_VERSION
  PATH "${IZ_MSVC_WINDOWS_SDK_ROOT}Include"
  DOC "Windows SDK")

set(windows.sdk.host "Host${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}")
set(windows.sdk.target "${CMAKE_VS_PLATFORM_NAME}")
set(msvc.tools.dir
    "${IZ_MSVC_INSTALL_PATH}/VC/Tools/MSVC/${IZ_MSVC_TOOLS_VERSION}")

message(STATUS "FOUND 1 ${windows.sdk.host}")
message(STATUS "FOUND 2 ${windows.sdk.target}")
message(STATUS "FOUND 3 ${msvc.tools.dir}")

# C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.43.34808\bin\Hostx64\x64
# C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/bin/Hostx64/x64
message(STATUS "Path 1 ${msvc.tools.dir}/bin/${windows.sdk.host}/${windows.sdk.target}")
message(STATUS "Path 2 ${IZ_MSVC_WINDOWS_SDK_ROOT}bin/${IZ_MSVC_WINDOWS_SDK_VERSION}/${windows.sdk.target}")
message(STATUS "Path 3 ${IZ_MSVC_WINDOWS_SDK_ROOT}bin")


block(SCOPE_FOR VARIABLES)
  list(PREPEND CMAKE_SYSTEM_PROGRAM_PATH
       "${msvc.tools.dir}/bin/${windows.sdk.host}/${windows.sdk.target}"
       "${IZ_MSVC_WINDOWS_SDK_ROOT}bin/${IZ_MSVC_WINDOWS_SDK_VERSION}/${windows.sdk.target}"
       "${IZ_MSVC_WINDOWS_SDK_ROOT}bin")
  find_program(CMAKE_MASM_ASM_COMPILER NAMES ml64 ml DOC "MSVC ASM Compiler")
  find_program(CMAKE_CXX_COMPILER NAMES cl REQUIRED DOC "MSVC C++ Compiler")
  find_program(CMAKE_RC_COMPILER NAMES rc REQUIRED DOC "MSVC Resource Compiler")
  find_program(CMAKE_C_COMPILER NAMES cl REQUIRED DOC "MSVC C Compiler")
  find_program(CMAKE_LINKER NAMES link REQUIRED DOC "MSVC Linker")
  find_program(CMAKE_AR NAMES lib REQUIRED DOC "MSVC Archiver")
  find_program(CMAKE_MT NAMES mt REQUIRED DOC "MSVC Manifest Tool")
endblock()

set(includes ucrt shared um winrt cppwinrt)
set(libs ucrt um)

list(TRANSFORM includes PREPEND "${IZ_MSVC_WINDOWS_SDK_ROOT}/Include/${IZ_MSVC_WINDOWS_SDK_VERSION}/")
list(TRANSFORM libs PREPEND "${IZ_MSVC_WINDOWS_SDK_ROOT}/Lib/${IZ_MSVC_WINDOWS_SDK_VERSION}/")
list(TRANSFORM libs APPEND "/${windows.sdk.target}")

# We could technically set `CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES` and others,
# but not for the library paths.
include_directories(BEFORE SYSTEM "${msvc.tools.dir}/include" ${includes})
link_directories(BEFORE "${msvc.tools.dir}/lib/${windows.sdk.target}" ${libs})

message(STATUS "All good")
