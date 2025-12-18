file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(PiCAN_Core STATIC ${CORE_SOURCES})

target_link_libraries(PiCAN_Core PUBLIC
        fmt::fmt
        spdlog::spdlog
        magic_enum::magic_enum
)

# This exposes the directory to anyone linking against this library
target_include_directories(PiCAN_Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)