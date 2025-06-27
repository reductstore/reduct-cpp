enable_testing()

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
    reduct/replication_api_test.cc
    reduct/server_api_test.cc
    reduct/token_api_test.cc
    test.cc
)

add_executable(reduct-tests ${SRC_FILES})
target_include_directories(reduct-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    reduct-tests
    PRIVATE ${RCPP_TARGET_NAME} fmt::fmt Catch2::Catch2
)
set_target_properties(
    reduct-tests
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
