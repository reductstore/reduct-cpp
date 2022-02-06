set(SRC_FILES
        reduct/internal/http_client.cc
        reduct/bucket.cc
        reduct/client.cc
        reduct/error.cc)

set(PUBLIC_HEADERS
        reduct/client.h
        reduct/error.h
        reduct/result.h)

add_library(reduct STATIC ${SRC_FILES})
target_link_libraries(reduct dependencies)
set_target_properties(reduct PROPERTIES PUBLIC_HEADER PUBLIC_HEADERS)