file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE MODULES "*.cppm")

add_library(heap STATIC ${SOURCES})

if(MODULES)
    target_sources(heap PUBLIC FILE_SET CXX_MODULES FILES ${MODULES})
endif()

target_link_libraries(heap PUBLIC stacktrace)
target_include_directories(heap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
