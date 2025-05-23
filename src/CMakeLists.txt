set(SRC_FILES
    reduct/internal/http_client.cc
    reduct/internal/serialisation.cc
    reduct/bucket.cc
    reduct/client.cc
    reduct/error.cc
)

set(PUBLIC_HEADERS
    reduct/bucket.h
    reduct/client.h
    reduct/error.h
    reduct/http_options.h
    reduct/result.h
    reduct/diagnostics.h
)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_FILES})
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../3dparty
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dependencies)
set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}"
)
target_compile_definitions(
    ${CMAKE_PROJECT_NAME}
    PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT
)
target_compile_definitions(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
        REDUCT_CPP_MAJOR_VERSION=${MAJOR_VERSION}
        REDUCT_CPP_MINOR_VERSION=${MINOR_VERSION}
)

if(REDUCT_CPP_USE_STD_CHRONO)
    target_compile_definitions(
        ${CMAKE_PROJECT_NAME}
        PRIVATE REDUCT_CPP_USE_STD_CHRONO
    )
endif()

include(GNUInstallDirs)
install(
    TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/reduct
)

install(
    FILES
        "${CMAKE_BINARY_DIR}/cmake/ReductCppConfig.cmake"
        "${CMAKE_BINARY_DIR}/cmake/ReductCppConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/${PROJECT_NAME}"
)
