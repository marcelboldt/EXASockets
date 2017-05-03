
add_subdirectory(../../websockets/websockets CMAKE_BINARY_DIR)


find_package(RapidJSON REQUIRED)

OPTION(USE_OpenMP "Use OpenMP" ON)
IF (USE_OpenMP)
    FIND_PACKAGE(OpenMP REQUIRED)
    IF (OPENMP_FOUND)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    ENDIF ()
ENDIF ()

find_package(OpenSSL REQUIRED)

set(SOURCE_FILES
        exasockets.h exasockets.cpp
        osname.h
        exaResultSet.h exaResultSet.cpp
        )
add_library(exasockets ${SOURCE_FILES})

set(LINK_LIBS
        websockets
        ${RAPIDJSON_LIBRARIES}
        ${RAPIDJSON_LFLAGS}
        ${OPENSSL_LIBRARIES}
        ${OpenMP_CXX_FLAGS}
        )
target_link_libraries(exasockets ${LINK_LIBS})