file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE MODULES "*.cppm")

add_library(stacktrace STATIC ${SOURCES})

if(MODULES)
    target_sources(stacktrace PUBLIC FILE_SET CXX_MODULES FILES ${MODULES})
endif()

target_link_libraries(stacktrace PUBLIC libbacktrace)
target_include_directories(stacktrace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
