set(TEST_EXECUTABLE "pican-test")

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE TEST_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})

target_sources(${TEST_EXECUTABLE} PRIVATE
        FILE_SET CXX_MODULES
        FILES ${TEST_MODULES}
)

set_target_properties(${TEST_EXECUTABLE} PROPERTIES
        CXX_STANDARD 23
        CXX_EXTENSIONS OFF
)

target_link_libraries(${TEST_EXECUTABLE} PRIVATE
        STACKTRACE_LIB
        HEAP_LIB
        PICAN_LIB
        Catch2::Catch2
)

# So that we can use #include "test/TestUtils.hpp" rather than relying on relative paths
target_include_directories(${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${TEST_EXECUTABLE})

target_compile_options(${TEST_EXECUTABLE} PRIVATE "-Wno-comment")

target_link_options(${TEST_EXECUTABLE} PRIVATE
        "LINKER:--wrap=malloc"
        "LINKER:--wrap=free"
        "LINKER:--wrap=calloc"
        "LINKER:--wrap=realloc"
        "LINKER:--wrap=aligned_alloc"
)