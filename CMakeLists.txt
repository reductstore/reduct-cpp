cmake_minimum_required(VERSION 3.18)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 4)
set(PATCH_VERSION 0)
set(REDUCT_CPP_FULL_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

project(reductcpp VERSION ${REDUCT_CPP_FULL_VERSION})

message(STATUS "Version ${REDUCT_CPP_FULL_VERSION}")

set(REDUCT_CPP_ENABLE_TESTS ON CACHE BOOL "Compile tests")
set(REDUCT_CPP_ENABLE_EXAMPLES ON CACHE BOOL "Compile examples")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(cmake/InstallDependencies.cmake)

configure_file(cmake/ReductCppConfig.cmake.in ${CMAKE_BINARY_DIR}/cmake/ReductCppConfig.cmake @ONLY)
configure_file(cmake/ReductCppConfigVersion.cmake.in ${CMAKE_BINARY_DIR}/cmake/ReductCppConfigVersion.cmake @ONLY)


include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/3dparty)

add_subdirectory(src)
if (REDUCT_CPP_ENABLE_TESTS)
    add_subdirectory(tests)
endif ()