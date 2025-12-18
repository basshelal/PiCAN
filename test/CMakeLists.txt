set(TEST_EXECUTABLE "PiCAN_Test")

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})

# Link against the Core Logic and GoogleTest
target_link_libraries(${TEST_EXECUTABLE} PRIVATE
        PiCAN_Core
        GTest::gtest_main
)

# So that we can use #include "test/TestUtils.hpp" rather than relying on relative paths
target_include_directories(PiCAN_Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(GoogleTest)
gtest_discover_tests(${TEST_EXECUTABLE})
