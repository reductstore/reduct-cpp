cmake_minimum_required(VERSION 3.18)

project(ReductCppExamples)

find_package(ZLIB)
find_package(OpenSSL)

find_package(reductcpp 1.15.0)

add_executable(usage-example usage_example.cc)
add_executable(subscription subscription.cc)
target_link_libraries(
    usage-example
    reductcpp::reductcpp
    OpenSSL::SSL
    OpenSSL::Crypto
)
target_link_libraries(
    subscription
    reductcpp::reductcpp
    OpenSSL::SSL
    OpenSSL::Crypto
)
