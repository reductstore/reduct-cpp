# if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
#     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
#     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
#             "${CMAKE_BINARY_DIR}/conan.cmake"
#             EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
#             TLS_VERIFY ON)
# endif ()

# include(${CMAKE_BINARY_DIR}/conan.cmake)

find_program(CONAN_CMD conan)
if(CONAN_CMD)
    find_package(fmt REQUIRED)
    find_package(nlohmann_json REQUIRED)
    find_package(httplib REQUIRED)
    find_package(concurrentqueue REQUIRED)

    add_library(dependencies INTERFACE)
    target_link_libraries(
        dependencies
        INTERFACE
            fmt::fmt
            nlohmann_json::nlohmann_json
            httplib::httplib
            concurrentqueue::concurrentqueue
    )
else()
    message(STATUS "Conan not found. Fetch dependencies")
    include(FetchContent)
    FetchContent_Declare(
        fmt
        URL https://github.com/fmtlib/fmt/archive/refs/tags/11.0.2.zip
        URL_HASH MD5=6e20923e12c4b78a99e528c802f459ef
    )

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
        URL_HASH MD5=23712ebf3a4b4ccb39f2375521716ab3
    )

    FetchContent_Declare(
        httplib
        URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.16.0.zip
        URL_HASH MD5=c5367889819d677bd06d6c7739896b2b
    )

    FetchContent_Declare(
        concurrentqueue
        URL
            https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.4.zip
        URL_HASH MD5=814c5e121b29e37ee836312f0eb0328f
    )

    add_library(dependencies INTERFACE)
    FetchContent_MakeAvailable(fmt nlohmann_json httplib concurrentqueue)
    target_link_libraries(
        dependencies
        INTERFACE fmt nlohmann_json httplib concurrentqueue
    )

    if(NOT REDUCT_CPP_USE_STD_CHRONO)
        FetchContent_Declare(
            date
            URL
                https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.zip
            URL_HASH MD5=cf556cc376d15055b8235b05b2fc6253
        )
        FetchContent_MakeAvailable(date)
        target_link_libraries(dependencies INTERFACE date)
    endif()
endif()
