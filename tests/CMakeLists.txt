enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set (SRC_FILES
        reduct/bucket_api_test.cc
        reduct/server_api_test.cc
        test.cc)

add_executable(reduct-tests ${SRC_FILES})
target_link_libraries(reduct-tests reduct ${CONAN_LIBS})