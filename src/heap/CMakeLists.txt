file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.hpp")

# Not header-only, the malloc wrappers and operator new/delete replacements must be compiled once, see Heap.cpp
add_library(heap STATIC ${SOURCES})

target_sources(heap PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES ${HEADERS}
)

target_link_libraries(heap PUBLIC stacktrace)

# Compile each header on its own to check that it is self-contained, see src/CMakeLists.txt
set_target_properties(heap PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
