file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE MODULES "*.cppm")

add_library(contracts STATIC ${SOURCES})

if(MODULES)
    target_sources(contracts PUBLIC FILE_SET CXX_MODULES FILES ${MODULES})
endif()

target_link_libraries(contracts PUBLIC stacktrace)
target_include_directories(contracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
