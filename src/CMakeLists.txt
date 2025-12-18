add_subdirectory(core)
add_subdirectory(qtui)

set(MAIN_EXECUTABLE "PiCAN")

add_executable(${MAIN_EXECUTABLE} main.cpp)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE
        PiCAN_Core
        PiCAN_QTUI
)

install(TARGETS ${MAIN_EXECUTABLE}
        RUNTIME DESTINATION bin
)
