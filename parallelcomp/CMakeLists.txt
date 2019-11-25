set(LIB_NAME parallelcomp)

set(CPP_FILES
        iterator.cpp
        )

set(CPP_HEADER_DIR include)

set(CPP_HEADERS
        ${CPP_HEADER_DIR}/iterator.h
        )

add_library(${LIB_NAME} STATIC ${CPP_FILES} ${CPP_HEADERS})
