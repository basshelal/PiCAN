add_subdirectory(stacktrace)
add_subdirectory(heap)
add_subdirectory(pican)

set(MAIN_EXECUTABLE "pican-app")

add_executable(${MAIN_EXECUTABLE} main.cppm)

#target_sources(${MAIN_EXECUTABLE}
#        PRIVATE
#        FILE_SET CXX_MODULES FILES main.cppm
#)

#set_target_properties(${MAIN_EXECUTABLE} PROPERTIES CXX_MODULE_STD ON)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE
        PICAN_LIB
        HEAP_LIB
        STACKTRACE_LIB
)

target_link_options(${MAIN_EXECUTABLE} PRIVATE
        "LINKER:--wrap=malloc"
        "LINKER:--wrap=free"
        "LINKER:--wrap=calloc"
        "LINKER:--wrap=realloc"
        "LINKER:--wrap=aligned_alloc"
)

install(TARGETS ${MAIN_EXECUTABLE}
        RUNTIME DESTINATION bin
)
