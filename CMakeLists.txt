cmake_minimum_required(VERSION 3.21)

project(2048 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(2048
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/gameboard.cpp
    src/gameboard.h
    src/constants.h
)

target_link_libraries(2048 PRIVATE Qt6::Widgets)

install(TARGETS 2048
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
