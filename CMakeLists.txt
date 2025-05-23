cmake_minimum_required(VERSION 3.23)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 15)
set(PATCH_VERSION 0)
set(REDUCT_CPP_FULL_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

project(reductcpp VERSION ${REDUCT_CPP_FULL_VERSION})

message(STATUS "Version ${REDUCT_CPP_FULL_VERSION}")

set(REDUCT_CPP_ENABLE_TESTS OFF CACHE BOOL "Compile tests")
set(REDUCT_CPP_USE_STD_CHRONO
    OFF
    CACHE BOOL
    "use std::chrono instead of date library"
)
set(REDUCT_CPP_USE_CONAN OFF CACHE BOOL "use conan to fetch dependencies")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(cmake/InstallDependencies.cmake)

configure_file(
    cmake/ReductCppConfig.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/ReductCppConfig.cmake
    @ONLY
)
configure_file(
    cmake/ReductCppConfigVersion.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/ReductCppConfigVersion.cmake
    @ONLY
)

add_subdirectory(src)
if(REDUCT_CPP_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
