file(GLOB_RECURSE PICAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE PICAN_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")

# TODO remove if we are 100% modules only
add_library(PICAN_LIB STATIC ${PICAN_SOURCES})

target_sources(PICAN_LIB
        PUBLIC
        FILE_SET CXX_MODULES FILES ${PICAN_MODULES}
)

target_link_libraries(PICAN_LIB PUBLIC
        STACKTRACE_LIB
        HEAP_LIB
        fmt::fmt-module
        magic_enum::magic_enum
)

# TODO remove if we are 100% modules only
target_include_directories(PICAN_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)