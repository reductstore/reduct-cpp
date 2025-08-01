if(REDUCT_CPP_USE_FETCHCONTENT)
    message(STATUS "Using Fetchcontent for dependencies")
    include(FetchContent)
    FetchContent_Declare(
        fmt
        URL https://github.com/fmtlib/fmt/archive/refs/tags/9.1.0.zip
        URL_HASH MD5=6e20923e12c4b78a99e528c802f459ef
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
        URL
            https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.4.zip
        URL_HASH MD5=814c5e121b29e37ee836312f0eb0328f
    )

    #    FetchContent_Declare(
    #        zlib
    #        URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib131.zip
    #        URL_HASH MD5=ef76f7ebfd97778a6653b1d8413541c0
    #    )

    FetchContent_MakeAvailable(
        fmt
        nlohmann_json
        httplib
        concurrentqueue
        #        zlib
    )

    # Add aliases
    add_library(concurrentqueue::concurrentqueue ALIAS concurrentqueue)
else()
    find_package(fmt 9.1.0 REQUIRED)
    find_package(nlohmann_json 3.11.3 REQUIRED)

    # If cpp-httplib is not found via find_package() search for it using pkg-config
    find_package(httplib 0.14.3 QUIET)
    if(NOT httplib_FOUND)
        message(
            STATUS
            "cpp-httplib not found via find_package(), checking pkg-config"
        )
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(httplib REQUIRED IMPORTED_TARGET cpp-httplib>=0.14.3)
        add_library(httplib::httplib ALIAS PkgConfig::httplib)
    endif()

    if(NOT VCPKG_ENABLED)
        find_package(concurrentqueue 1.0.4 REQUIRED)
    else()
        find_package(unofficial-concurrentqueue 1.0.4 REQUIRED)
        add_library(
            concurrentqueue::concurrentqueue
            ALIAS unofficial::concurrentqueue::concurrentqueue
        )
    endif()
endif()

find_package(OpenSSL 3.0.13 REQUIRED)

# Set dependencies list
set(RCPP_DEPENDENCIES
    fmt::fmt
    nlohmann_json::nlohmann_json
    httplib::httplib
    concurrentqueue::concurrentqueue
    OpenSSL::SSL
    OpenSSL::Crypto
)

# std::chrono
if(NOT REDUCT_CPP_USE_STD_CHRONO)
    message(STATUS "Using date library")

    if(REDUCT_CPP_USE_FETCHCONTENT)
        FetchContent_Declare(
            date
            URL
                https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.zip
            URL_HASH MD5=cf556cc376d15055b8235b05b2fc6253
        )
        FetchContent_MakeAvailable(date)
    else()
        find_package(date 3.0.1 REQUIRED)
    endif()

    list(APPEND RCPP_DEPENDENCIES date::date)
else()
    message(STATUS "Using std::chrono")
endif()
