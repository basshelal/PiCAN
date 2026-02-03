file(GLOB_RECURSE PICAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(PICAN_LIB STATIC ${PICAN_SOURCES})

target_link_libraries(PICAN_LIB PUBLIC
        fmt::fmt
        magic_enum::magic_enum
)

# This exposes the directory to anyone linking against this library
target_include_directories(PICAN_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)