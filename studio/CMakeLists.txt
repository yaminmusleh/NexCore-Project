cmake_minimum_required(VERSION 3.21)

project(nexcore-studio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(nexcore-studio
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/BuildController.h
    src/BuildController.cpp
    src/CodeEditor.h
    src/CodeEditor.cpp
    src/FindBar.h
    src/FindBar.cpp
    src/NexHighlighter.h
    src/NexHighlighter.cpp
    src/OutputPanel.h
    src/OutputPanel.cpp
    src/Sidebar.h
    src/Sidebar.cpp
)

target_link_libraries(nexcore-studio PRIVATE Qt6::Widgets)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nexcore-studio PRIVATE -Wall -Wextra)
endif()

# --- Installation (Linux) -----------------------------------------------------
#   cmake --install build-studio --prefix ~/.local
# installs the program, a launcher entry and the icon. The launcher's Exec line
# is written at install time, so it always holds the real absolute path.
install(TARGETS nexcore-studio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES packaging/nexcore-studio.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(CODE "
    set(NEXCORE_STUDIO_EXEC \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/nexcore-studio\")
    configure_file(\"${CMAKE_CURRENT_SOURCE_DIR}/packaging/nexcore-studio.desktop.in\"
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications/nexcore-studio.desktop\"
        @ONLY)
    # install(CODE) files are not in the manifest by default; add this one so
    # that install_manifest.txt lists everything that was installed.
    list(APPEND CMAKE_INSTALL_MANIFEST_FILES
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications/nexcore-studio.desktop\")
")