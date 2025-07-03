cmake_minimum_required(VERSION 3.23)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 15)
set(PATCH_VERSION 0)
set(REDUCT_CPP_FULL_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

project(reductcpp VERSION ${REDUCT_CPP_FULL_VERSION} LANGUAGES CXX)

message(STATUS "Version ${REDUCT_CPP_FULL_VERSION}")

# Use std::chrono
set(REDUCT_CPP_USE_STD_CHRONO
    OFF
    CACHE BOOL
    "use std::chrono instead of date library"
)
if(REDUCT_CPP_USE_STD_CHRONO)
    set(DATE_DEPENDENCY "")
else()
    set(DATE_DEPENDENCY "find_dependency(date REQUIRED)")
endif()

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

# Dependencies
include(cmake/InstallDependencies.cmake)

set(RCPP_TARGET_NAME ${PROJECT_NAME})
add_subdirectory(src)

# Tests
set(REDUCT_CPP_ENABLE_TESTS OFF CACHE BOOL "Compile tests")
if(REDUCT_CPP_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
