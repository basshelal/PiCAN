add_subdirectory(stacktrace)
add_subdirectory(heap)
add_subdirectory(contracts)
add_subdirectory(pican)

set(MAIN_EXECUTABLE "pican-app")

add_executable(${MAIN_EXECUTABLE} main.cpp)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE
        pican
)

target_link_options(${MAIN_EXECUTABLE} PRIVATE
        "LINKER:--wrap=malloc"
        "LINKER:--wrap=free"
        "LINKER:--wrap=calloc"
        "LINKER:--wrap=realloc"
        "LINKER:--wrap=aligned_alloc"
        "--gc-sections"
)

# Header-only code is only compiled when something includes it, so a header that nothing includes yet could silently
# rot. VERIFY_INTERFACE_HEADER_SETS (set on each library) generates a target that compiles every header on its own,
# which also checks that each header is self-contained (includes everything it needs).
# Building the app (or the tests) builds these too.
add_dependencies(${MAIN_EXECUTABLE}
        stacktrace_verify_interface_header_sets
        contracts_verify_interface_header_sets
        heap_verify_interface_header_sets
        pican_verify_interface_header_sets
)

install(TARGETS ${MAIN_EXECUTABLE}
        RUNTIME DESTINATION bin
)
