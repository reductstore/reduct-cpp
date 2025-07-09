cmake_minimum_required(VERSION 3.18)

project(ReductCppExamples)

if(REDUCT_CPP_EXAMPLE_USE_FETCHCONTENT)
    include(FetchContent)

    # Enable using fetchcontent for reductcpp dependencies
    set(REDUCT_CPP_USE_FETCHCONTENT ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        reductcpp
        GIT_REPOSITORY https://github.com/reductstore/reduct-cpp
        GIT_TAG refactor-cmake
    )
    FetchContent_MakeAvailable(reductcpp)
else()
    find_package(reductcpp 1.15.0)
endif()

add_executable(usage-example usage_example.cc)
add_executable(subscription subscription.cc)
target_link_libraries(usage-example reductcpp::reductcpp)
target_link_libraries(subscription reductcpp::reductcpp)
