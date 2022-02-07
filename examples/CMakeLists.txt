cmake_minimum_required(VERSION 3.18)

project(ReductCppExamples)
set(CMAKE_CXX_STANDARD 20)


find_package(ReductCpp 0.1.0)

add_executable(usage-example usage_example.cc)
target_link_libraries(usage-example ${REDUCT_CPP_LIBRARIES})
