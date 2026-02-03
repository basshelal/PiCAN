add_subdirectory(stacktrace)
add_subdirectory(noheap)
add_subdirectory(pican)

set(MAIN_EXECUTABLE "pican-app")

add_executable(${MAIN_EXECUTABLE} main.cpp)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE
        PICAN_LIB
        NOHEAP_LIB
        STACKTRACE_LIB
)

target_link_options(${MAIN_EXECUTABLE} PRIVATE
        "LINKER:--wrap=malloc"
        "LINKER:--wrap=free"
        "LINKER:--wrap=calloc"
        "LINKER:--wrap=realloc"
)

install(TARGETS ${MAIN_EXECUTABLE}
        RUNTIME DESTINATION bin
)
