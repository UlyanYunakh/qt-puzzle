cmake_minimum_required(VERSION 3.16)

project(Puzzle VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick)

set(PROJECT_SOURCES
        main.cpp
        src/Items/PuzzleCanvas/puzzlecanvas.cpp
        src/Items/PuzzleCanvas/puzzlecanvas.h
        src/PuzzleGeometry/puzzlegeometrygenerator.cpp
        src/PuzzleGeometry/puzzlegeometrygenerator.h
        src/PuzzleGeometry/puzzleshapeloader.cpp
        src/PuzzleGeometry/puzzleshapeloader.h
        src/PuzzleGeometry/puzzleshapeset.h
)

qt_add_executable(Puzzle
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

set_source_files_properties(
        src/DefaultPuzzles/test-puzzle-by-daniel-leone.jpg
        PROPERTIES QT_RESOURCE_ALIAS test-puzzle.jpg
)

set_source_files_properties(
        src/PuzzleShapes/classic.json
        PROPERTIES QT_RESOURCE_ALIAS classic.json
)

qt_add_resources(Puzzle "puzzle_assets"
        PREFIX "/assets"
        FILES src/DefaultPuzzles/test-puzzle-by-daniel-leone.jpg
)

qt_add_resources(Puzzle "puzzle_shapes"
        PREFIX "/puzzle-shapes"
        FILES src/PuzzleShapes/classic.json
)

target_link_libraries(Puzzle PRIVATE Qt6::Quick)
target_include_directories(Puzzle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Keep the Windows build runnable without relying on a machine-wide PATH.
# windeployqt copies the Qt libraries, platform plugin, and MinGW runtime next
# to the executable whenever the target is linked.
if (WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIRECTORY "${QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS "${QT_BIN_DIRECTORY}"
            NO_DEFAULT_PATH
    )

    if (NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt was not found beside qmake: ${QT_BIN_DIRECTORY}")
    endif ()

    add_custom_command(TARGET Puzzle POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --compiler-runtime
            "$<TARGET_FILE:Puzzle>"
            COMMENT "Deploying Qt and MinGW runtime files"
            VERBATIM
    )
endif ()

set_target_properties(Puzzle PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Puzzle
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Puzzle
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(Puzzle)
