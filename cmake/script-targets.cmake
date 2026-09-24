set(SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/scripts/")

# clang-format-check
add_custom_target(
        clang-format-check
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/clang-format-check.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
)

# clang-tidy-check
add_custom_target(
        clang-tidy-check
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/clang-tidy-check.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
)

# semgrep-check
add_custom_target(
        semgrep-check
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/semgrep-check.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
)
