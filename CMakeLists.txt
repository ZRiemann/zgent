cmake_minimum_required(VERSION 3.20)

set(ZGENT_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
if(NOT EXISTS "${ZGENT_VERSION_FILE}")
    message(FATAL_ERROR "zgent VERSION file not found: ${ZGENT_VERSION_FILE}")
endif()

file(STRINGS "${ZGENT_VERSION_FILE}" ZGENT_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${ZGENT_VERSION_RAW}" ZGENT_VERSION_RAW)
if(NOT ZGENT_VERSION_RAW MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR
        "Invalid zgent VERSION value: ${ZGENT_VERSION_RAW}\n"
        "Expected format: vX.Y.Z, for example v0.1.0."
    )
endif()
set(ZGENT_PROJECT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")

project(zgent VERSION ${ZGENT_PROJECT_VERSION} LANGUAGES CXX)

option(ZGENT_BUILD_TESTS "Build zgent tests" ON)

set(ZETA_CMAKE_UTIL_DIR "" CACHE PATH "Path to zeta_forge/cmake_util")
if(NOT ZETA_CMAKE_UTIL_DIR AND DEFINED ENV{ZETAX_ROOT})
    set(_zgent_cmake_util "$ENV{ZETAX_ROOT}/zeta_forge/cmake_util")
    if(EXISTS "${_zgent_cmake_util}/common.cmake")
        set(ZETA_CMAKE_UTIL_DIR "${_zgent_cmake_util}" CACHE PATH "Path to zeta_forge/cmake_util" FORCE)
    endif()
endif()
if(NOT ZETA_CMAKE_UTIL_DIR OR NOT EXISTS "${ZETA_CMAKE_UTIL_DIR}/common.cmake")
    message(FATAL_ERROR
        "zeta_forge/cmake_util not found. Configure the zeta_forge environment first, then build zgent with ./zbuild.py. "
        "For direct CMake usage, pass -DZETA_CMAKE_UTIL_DIR=/path/to/zeta_forge/cmake_util."
    )
endif()

include("${ZETA_CMAKE_UTIL_DIR}/common.cmake")

set(ZETA_DEPS_CMAKE_DIR "" CACHE PATH "Path to installed zeta_forge deps CMake package files")
if(NOT ZETA_DEPS_CMAKE_DIR)
    set(_zgent_install_prefix "$ENV{ZETA_INSTALL_PREFIX}")
    if(NOT _zgent_install_prefix)
        set(_zgent_install_prefix "$ENV{HOME}/.local")
    endif()
    set(_zgent_deps_candidate "${_zgent_install_prefix}/lib/cmake/zeta_deps/${CMAKE_BUILD_TYPE}")
    if(EXISTS "${_zgent_deps_candidate}")
        set(ZETA_DEPS_CMAKE_DIR "${_zgent_deps_candidate}" CACHE PATH
            "Path to installed zeta_forge deps CMake package files" FORCE)
    endif()
endif()
if(NOT ZETA_DEPS_CMAKE_DIR OR NOT EXISTS "${ZETA_DEPS_CMAKE_DIR}")
    message(FATAL_ERROR
        "zeta_forge deps package configs not found. Install the shared dependency environment first with: "
        "$ENV{ZETAX_ROOT}/zeta_forge/zbuild.py install deps --profile release (or debug), then build zgent with ./zbuild.py. "
        "For direct CMake usage, pass -DZETA_DEPS_CMAKE_DIR=/path/to/lib/cmake/zeta_deps/<BUILD_TYPE>."
    )
endif()

list(PREPEND CMAKE_PREFIX_PATH "${ZETA_DEPS_CMAKE_DIR}")
find_package(zpp CONFIG REQUIRED)
find_package(zpp_nng CONFIG REQUIRED)
find_package(zpp_hpx CONFIG REQUIRED)

if(ZGENT_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src)

if(ZGENT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
