set(SRC_FILES
        reduct/internal/http_client.cc
        reduct/bucket.cc
        reduct/client.cc
        reduct/error.cc)

set(PUBLIC_HEADERS
        reduct/bucket.h
        reduct/client.h
        reduct/error.h
        reduct/http_options.h
        reduct/result.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dependencies)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")


include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/reduct
        )

install(FILES
        "${CMAKE_BINARY_DIR}/cmake/ReductCppConfig.cmake"
        "${CMAKE_BINARY_DIR}/cmake/ReductCppConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/${PROJECT_NAME}")
