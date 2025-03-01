if(REDUCT_CPP_USE_CONAN)
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
    if(NOT REDUCT_CPP_USE_STD_CHRONO)
        message(STATUS "Using date library")

        find_package(date REQUIRED)
        target_link_libraries(dependencies INTERFACE date::date)
    else()
        message(STATUS "Using std::chrono")
    endif()
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

    FetchContent_Declare(
        zlib
        URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib131.zip
        URL_HASH MD5=ef76f7ebfd97778a6653b1d8413541c0
    )

    # use system OpenSSL
    find_package(OpenSSL REQUIRED)

    add_library(dependencies INTERFACE)
    FetchContent_MakeAvailable(
        fmt
        nlohmann_json
        httplib
        concurrentqueue
        zlib
    )
    target_link_libraries(
        dependencies
        INTERFACE
            fmt
            nlohmann_json
            httplib
            concurrentqueue
            zlib
            OpenSSL::SSL
            OpenSSL::Crypto
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
