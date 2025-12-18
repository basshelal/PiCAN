# Qt Settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup(REQUIRES 6.9)

file(GLOB_RECURSE QTUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(PiCAN_QTUI STATIC ${QTUI_SOURCES})

target_link_libraries(PiCAN_QTUI PRIVATE
        PiCAN_Core
        Qt6::Core
        Qt6::Quick
)

# Allows: #include "ui-qt/example.h"
target_include_directories(PiCAN_QTUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)