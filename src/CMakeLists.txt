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

# Create reductcpp target
add_library(${RCPP_TARGET_NAME} STATIC ${SRC_FILES})

# Add alias
add_library(${RCPP_TARGET_NAME}::${RCPP_TARGET_NAME} ALIAS ${RCPP_TARGET_NAME})

# Set c++ standard
target_compile_features(${RCPP_TARGET_NAME} PUBLIC cxx_std_20)

set_property(TARGET ${RCPP_TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Include paths
target_include_directories(
    ${RCPP_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link dependencies
target_link_libraries(${RCPP_TARGET_NAME} PRIVATE ${RCPP_DEPENDENCIES})

set_target_properties(
    ${RCPP_TARGET_NAME}
    PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

# Add definitions
target_compile_definitions(
    ${RCPP_TARGET_NAME}
    PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT
)
target_compile_definitions(
    ${RCPP_TARGET_NAME}
    PUBLIC
        REDUCT_CPP_MAJOR_VERSION=${MAJOR_VERSION}
        REDUCT_CPP_MINOR_VERSION=${MINOR_VERSION}
)

if(REDUCT_CPP_USE_STD_CHRONO)
    target_compile_definitions(
        ${RCPP_TARGET_NAME}
        PRIVATE REDUCT_CPP_USE_STD_CHRONO
    )
endif()

if(RCPP_INSTALL)
    # Install rules
    include(GNUInstallDirs)

    # Install
    install(
        TARGETS ${RCPP_TARGET_NAME}
        EXPORT ${RCPP_TARGET_NAME}-target
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/reduct
    )

    install(
        EXPORT ${RCPP_TARGET_NAME}-target
        FILE ${RCPP_TARGET_NAME}-targets.cmake
        NAMESPACE ${RCPP_TARGET_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${RCPP_TARGET_NAME}/cmake
    )

    include(CMakePackageConfigHelpers)

    # Create package config file
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${RCPP_TARGET_NAME}-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${RCPP_TARGET_NAME}-config.cmake"
        INSTALL_DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/${RCPP_TARGET_NAME}/cmake
    )
    # Create package version file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${RCPP_TARGET_NAME}-config-version.cmake
        COMPATIBILITY ExactVersion
    )

    # Install cmake files
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${RCPP_TARGET_NAME}-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${RCPP_TARGET_NAME}-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${RCPP_TARGET_NAME}/cmake
    )
endif()
