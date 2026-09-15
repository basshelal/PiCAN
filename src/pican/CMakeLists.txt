file(GLOB_RECURSE PICAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE PICAN_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")

add_library(pican STATIC ${PICAN_SOURCES})

target_sources(pican
        PUBLIC
        FILE_SET CXX_MODULES FILES ${PICAN_MODULES}
)

target_link_libraries(pican PUBLIC
        stacktrace
        heap
        contracts
        fmt::fmt-module
        magic_enum::magic_enum
)

target_include_directories(pican PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)