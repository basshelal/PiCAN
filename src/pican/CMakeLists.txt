file(GLOB_RECURSE PICAN_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# Header-only
add_library(pican INTERFACE)

target_sources(pican INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES ${PICAN_HEADERS}
)

target_link_libraries(pican INTERFACE
        stacktrace
        heap
        contracts
        fmt::fmt
        magic_enum::magic_enum
)

# Compile each header on its own to check that it is self-contained, see src/CMakeLists.txt
set_target_properties(pican PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
