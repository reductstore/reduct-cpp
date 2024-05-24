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
            URL https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.zip
            URL_HASH MD5=1bba4e8bdd7b0fa98f207559ffa380a3
    )

    FetchContent_Declare(
            nlohmann_json
            URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
            URL_HASH MD5=23712ebf3a4b4ccb39f2375521716ab3
    )

    FetchContent_Declare(
            httplib
            URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.14.3.zip
            URL_HASH MD5=af82eb38506ca531b6d1d53524ff7912
    )

    FetchContent_Declare(
            concurrentqueue
            URL https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.4.zip
            URL_HASH MD5=814c5e121b29e37ee836312f0eb0328f
    )

    add_library(dependencies INTERFACE)
    FetchContent_MakeAvailable(fmt nlohmann_json httplib concurrentqueue)
    target_link_libraries(dependencies INTERFACE fmt nlohmann_json httplib concurrentqueue)

    if(NOT REDUCT_CPP_USE_STD_CHRONO)
        FetchContent_Declare(
                date
                URL https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.zip
                URL_HASH MD5=cf556cc376d15055b8235b05b2fc6253)
        FetchContent_MakeAvailable(date)
        target_link_libraries(dependencies INTERFACE date)
    endif()
endif ()
