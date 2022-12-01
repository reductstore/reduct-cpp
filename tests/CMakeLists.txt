enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.8.zip
        URL_HASH MD5=78148e1a75aea786038fb8d21b9455f2
)
FetchContent_MakeAvailable(Catch2)


set(SRC_FILES
        reduct/bucket_api_test.cc
        reduct/entry_api_test.cc
        reduct/server_api_test.cc
        test.cc reduct/token_api_test.cc)

add_executable(reduct-tests ${SRC_FILES})

target_link_libraries(reduct-tests ${CMAKE_PROJECT_NAME} Catch2::Catch2)
