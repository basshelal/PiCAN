add_subdirectory(pican)

set(MAIN_EXECUTABLE "pican-app")

add_executable(${MAIN_EXECUTABLE} main.cpp)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE
        PICAN_LIB
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
