cmake_minimum_required(VERSION 3.16)

project(kijitabu)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(kijitabu
    src/main.cpp
    src/EditorWindow.cpp
    src/CodeEditor.cpp
    src/LineNumberArea.cpp
)

target_link_libraries(kijitabu PRIVATE Qt6::Widgets)
