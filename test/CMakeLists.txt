set(TEST_EXECUTABLE "pican-test")

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})

target_link_libraries(${TEST_EXECUTABLE} PRIVATE
        PICAN_LIB
        STACKTRACE_LIB
        GTest::gtest
)

# So that we can use #include "test/TestUtils.hpp" rather than relying on relative paths
target_include_directories(${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(GoogleTest)
gtest_discover_tests(${TEST_EXECUTABLE})

target_link_options(${TEST_EXECUTABLE} PRIVATE
        "LINKER:--wrap=malloc"
        "LINKER:--wrap=free"
        "LINKER:--wrap=calloc"
        "LINKER:--wrap=realloc"
)