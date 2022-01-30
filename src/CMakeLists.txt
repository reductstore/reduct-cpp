set(SRC_FILES
        reduct/client.cc
        reduct/error.cc)

set(HEADERS
        reduct/client.h
        reduct/error.h)
add_library(reduct STATIC ${SRC_FILES})
set_target_properties(reduct PROPERTIES PUBLIC_HEADER HEADERS)