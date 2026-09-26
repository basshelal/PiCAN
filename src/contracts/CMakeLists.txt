file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.hpp")

# Header-only
add_library(contracts INTERFACE)

target_sources(contracts INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES ${HEADERS}
)

target_link_libraries(contracts INTERFACE stacktrace)

# Compile each header on its own to check that it is self-contained, see src/CMakeLists.txt
set_target_properties(contracts PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
