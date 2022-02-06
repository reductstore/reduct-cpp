cmake_minimum_required(VERSION 3.18)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PATCH_VERSION 0)
set(FULL_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

project(reduct_cpp VERSION ${FULL_VERSION})

message(STATUS "Version ${FULL_VERSION}")

set(REDUCT_CPP_ENABLE_TESTS ON CACHE BOOL "Compile tests")
set(REDUCT_CPP_ENABLE_EXAMPLES ON CACHE BOOL "Compile examples")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
            TLS_VERIFY ON)
endif ()

include(${CMAKE_BINARY_DIR}/conan.cmake)

find_program(CONAN_CMD conan)
if (CONAN_CMD)
    conan_cmake_autodetect(settings)
    conan_cmake_install(PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}/conanfile.txt
            BUILD missing
            SETTINGS ${settings})

    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()

    add_library(dependencies INTERFACE)
    target_link_libraries(dependencies INTERFACE ${CONAN_LIBS})
else ()
    message(STATUS "Conan not found. Fetch dependencies")
    include(FetchContent)
    FetchContent_Declare(
            fmt
            URL https://github.com/fmtlib/fmt/archive/refs/tags/8.1.1.zip
            URL_HASH MD5=fed2f2c5027a4034cc8351bf59aa8f7c
    )

    FetchContent_Declare(
            nlohmann_json
            URL https://github.com/nlohmann/json/archive/refs/tags/v3.10.5.zip
            URL_HASH MD5=accaeb6a75f5972f479ef9139fa65b9e
    )

    FetchContent_Declare(
            httplib
            URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.10.1.zip
            URL_HASH MD5=a0a3b8852f2ddd68a8fa428643710b87
    )

    FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.8.zip
            URL_HASH MD5=78148e1a75aea786038fb8d21b9455f2
    )

    FetchContent_MakeAvailable(fmt nlohmann_json httplib Catch2)
    add_library(dependencies INTERFACE)
    target_link_libraries(dependencies INTERFACE fmt nlohmann_json httplib::httplib Catch2::Catch2)
endif ()

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
if (REDUCT_CPP_ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

if (REDUCT_CPP_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif ()