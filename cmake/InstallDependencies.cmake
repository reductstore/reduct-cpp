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
    conan_cmake_install(PATH_OR_REFERENCE ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.py
            BUILD missing
            SETTINGS ${settings})

    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()

    add_library(dependencies INTERFACE)
    target_link_libraries(dependencies INTERFACE ${CONAN_LIBS})
    target_compile_definitions(dependencies INTERFACE CONAN)

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
            URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.10.7.zip
            URL_HASH MD5=31497d5f3ff1e0df2f57195dbabd3198
    )

    FetchContent_Declare(
            concurrentqueue
            URL https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.3.zip
            URL_HASH MD5=6e879b14c833df7c011be5959e70cef7
    )

    FetchContent_Declare(
            date
            URL https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.zip
            URL_HASH MD5=cf556cc376d15055b8235b05b2fc6253)

    FetchContent_MakeAvailable(fmt nlohmann_json httplib concurrentqueue date)
    add_library(dependencies INTERFACE)
    target_link_libraries(dependencies INTERFACE fmt nlohmann_json httplib concurrentqueue date)
endif ()
