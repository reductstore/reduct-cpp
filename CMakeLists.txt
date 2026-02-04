cmake_minimum_required(VERSION 3.23)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 18)
set(PATCH_VERSION 0)
set(REDUCT_CPP_FULL_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

project(reductcpp VERSION ${REDUCT_CPP_FULL_VERSION} LANGUAGES CXX)

message(STATUS "Version ${REDUCT_CPP_FULL_VERSION}")

# Use FetchContent
set(REDUCT_CPP_USE_FETCHCONTENT
    OFF
    CACHE BOOL
    "Use fetchcontent to fetch dependencies"
)

# Set RCPP_INSTALL
if(REDUCT_CPP_USE_FETCHCONTENT)
    set(RCPP_INSTALL OFF)
else()
    set(RCPP_INSTALL ON)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

# Detect whether vcpkg is being used by checking CMAKE_TOOLCHAIN_FILE
if(
    DEFINED CMAKE_TOOLCHAIN_FILE
    AND "${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg\\.cmake$"
)
    set(VCPKG_ENABLED
        TRUE
        CACHE INTERNAL
        "Whether vcpkg integration is enabled"
    )
    message(
        STATUS
        "vcpkg detected via CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    )
else()
    set(VCPKG_ENABLED
        FALSE
        CACHE INTERNAL
        "Whether vcpkg integration is enabled"
    )
endif()

# Dependencies
include(cmake/InstallDependencies.cmake)

set(RCPP_TARGET_NAME ${PROJECT_NAME})
add_subdirectory(src)

# Tests
set(REDUCT_CPP_ENABLE_TESTS OFF CACHE BOOL "Compile tests")
if(REDUCT_CPP_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
