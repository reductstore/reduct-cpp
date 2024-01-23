cmake_minimum_required(VERSION 3.18)

project(ReductCppExamples)
set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB)
find_package(OpenSSL)



find_package(ReductCpp 1.8.0)

add_executable(usage-example usage_example.cc)
add_executable(subscription subscription.cc)
target_link_libraries(usage-example ${REDUCT_CPP_LIBRARIES} ${ZLIB_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(subscription ${REDUCT_CPP_LIBRARIES} ${ZLIB_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
