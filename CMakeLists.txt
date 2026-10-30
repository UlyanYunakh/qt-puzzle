cmake_minimum_required(VERSION 3.16)

project(Puzzle VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Multimedia Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

set(PROJECT_SOURCES
        main.cpp
        src/Audio/puzzlesoundsystem.cpp
        src/Audio/puzzlesoundsystem.h
        src/Config/puzzleconfig.cpp
        src/Config/puzzleconfig.h
        src/Items/PuzzleCanvas/puzzlecanvas.cpp
        src/Items/PuzzleCanvas/puzzlecanvas.h
        src/PuzzleLogic/puzzlelogic.cpp
        src/PuzzleLogic/puzzlelogic.h
        src/PuzzleGeometry/puzzlegeometrygenerator.cpp
        src/PuzzleGeometry/puzzlegeometrygenerator.h
        src/PuzzleGeometry/puzzleshapeloader.cpp
        src/PuzzleGeometry/puzzleshapeloader.h
        src/PuzzleGeometry/puzzleshapeset.h
        src/Settings/puzzleapplicationsettings.cpp
        src/Settings/puzzleapplicationsettings.h
        src/SaveGame/puzzlesavegamesystem.cpp
        src/SaveGame/puzzlesavegamesystem.h
)

qt_add_executable(Puzzle
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

set_source_files_properties(src/UI/Main.qml PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(Puzzle
        URI Puzzle.UI
        VERSION 1.0
        QML_FILES src/UI/Main.qml
)

set_source_files_properties(
        src/DefaultPuzzles/pexels-andy-lee-222330306-35678159.jpg
        PROPERTIES QT_RESOURCE_ALIAS test-puzzle.jpg
)

set_source_files_properties(
        src/PuzzleShapes/classic.json
        PROPERTIES QT_RESOURCE_ALIAS classic.json
)

qt_add_resources(Puzzle "puzzle_assets"
        PREFIX "/assets"
        FILES src/DefaultPuzzles/pexels-andy-lee-222330306-35678159.jpg
)

set_source_files_properties(src/SFX/piece-drop.wav
        PROPERTIES QT_RESOURCE_ALIAS piece-drop.wav)
set_source_files_properties(src/SFX/piece-placed.wav
        PROPERTIES QT_RESOURCE_ALIAS piece-placed.wav)
set_source_files_properties(src/SFX/button-click.wav
        PROPERTIES QT_RESOURCE_ALIAS button-click.wav)
set_source_files_properties(src/SFX/button-click-v2.wav
        PROPERTIES QT_RESOURCE_ALIAS button-click-v2.wav)

qt_add_resources(Puzzle "puzzle_sounds"
        PREFIX "/sfx"
        FILES
        src/SFX/piece-drop.wav
        src/SFX/piece-placed.wav
        src/SFX/button-click.wav
        src/SFX/button-click-v2.wav
)

qt_add_resources(Puzzle "puzzle_shapes"
        PREFIX "/puzzle-shapes"
        FILES src/PuzzleShapes/classic.json
)

set_source_files_properties(
        src/Config/puzzle-config.json
        PROPERTIES QT_RESOURCE_ALIAS puzzle-config.json
)

qt_add_resources(Puzzle "puzzle_config"
        PREFIX "/config"
        FILES src/Config/puzzle-config.json
)

target_link_libraries(Puzzle PRIVATE Qt6::Multimedia Qt6::Quick Qt6::QuickControls2)
target_include_directories(Puzzle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(Puzzle PRIVATE
        PUZZLE_VERSION="${PROJECT_VERSION}"
)

set_target_properties(Puzzle PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER com.puzzle.app
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

# Generate a self-contained installation. On Windows this invokes Qt's
# deployment tooling with the QML import information collected for Puzzle, so
# the package includes Qt DLLs, plugins, QML modules, the multimedia backend,
# and the compiler runtime needed on a machine without Qt installed.
qt_generate_deploy_qml_app_script(
        TARGET Puzzle
        OUTPUT_SCRIPT puzzle_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${puzzle_deploy_script})

if (WIN32)
    # CPack's archive generator treats square brackets in a source path as a
    # pattern. Package from inside a staging directory so projects living in a
    # path such as "[Qt] Puzzle" still produce a complete archive.
    set(PUZZLE_PACKAGE_NAME "Puzzle-${PROJECT_VERSION}-windows-x64")
    set(PUZZLE_PACKAGE_STAGING_DIR "${CMAKE_BINARY_DIR}/package")
    set(PUZZLE_PACKAGE_ROOT
            "${PUZZLE_PACKAGE_STAGING_DIR}/${PUZZLE_PACKAGE_NAME}")
    set(PUZZLE_PACKAGE_ARCHIVE
            "${CMAKE_BINARY_DIR}/${PUZZLE_PACKAGE_NAME}.zip")

    add_custom_target(portable-package
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
            "${PUZZLE_PACKAGE_STAGING_DIR}"
            COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --prefix "${PUZZLE_PACKAGE_ROOT}"
            COMMAND "${CMAKE_COMMAND}" -E chdir
            "${PUZZLE_PACKAGE_STAGING_DIR}"
            "${CMAKE_COMMAND}" -E tar cf
            "${PUZZLE_PACKAGE_ARCHIVE}"
            --format=zip "${PUZZLE_PACKAGE_NAME}"
            DEPENDS Puzzle
            COMMENT "Creating self-contained ${PUZZLE_PACKAGE_NAME}.zip"
            VERBATIM
    )
endif ()

if (BUILD_TESTING)
    add_executable(PuzzleLogicTests
            tests/puzzlelogic_tests.cpp
            src/PuzzleLogic/puzzlelogic.cpp
            src/PuzzleLogic/puzzlelogic.h
    )
    target_include_directories(PuzzleLogicTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME PuzzleLogicTests COMMAND PuzzleLogicTests)

    add_executable(PuzzleCanvasTests
            tests/puzzlecanvas_tests.cpp
            src/Config/puzzleconfig.cpp
            src/Config/puzzleconfig.h
            src/Items/PuzzleCanvas/puzzlecanvas.cpp
            src/Items/PuzzleCanvas/puzzlecanvas.h
            src/PuzzleLogic/puzzlelogic.cpp
            src/PuzzleLogic/puzzlelogic.h
            src/PuzzleGeometry/puzzlegeometrygenerator.cpp
            src/PuzzleGeometry/puzzlegeometrygenerator.h
            src/PuzzleGeometry/puzzleshapeloader.cpp
            src/PuzzleGeometry/puzzleshapeloader.h
            src/PuzzleGeometry/puzzleshapeset.h
    )
    target_include_directories(PuzzleCanvasTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(PuzzleCanvasTests PRIVATE Qt6::Quick)
    qt_add_resources(PuzzleCanvasTests "test_puzzle_assets"
            PREFIX "/assets"
            FILES src/DefaultPuzzles/pexels-andy-lee-222330306-35678159.jpg
    )
    qt_add_resources(PuzzleCanvasTests "test_puzzle_shapes"
            PREFIX "/puzzle-shapes"
            FILES src/PuzzleShapes/classic.json
    )
    qt_add_resources(PuzzleCanvasTests "test_puzzle_config"
            PREFIX "/config"
            FILES src/Config/puzzle-config.json
    )
    qt_add_resources(PuzzleCanvasTests "test_puzzle_sounds"
            PREFIX "/sfx"
            FILES
            src/SFX/piece-drop.wav
            src/SFX/piece-placed.wav
            src/SFX/button-click.wav
            src/SFX/button-click-v2.wav
    )
    add_test(NAME PuzzleCanvasTests COMMAND PuzzleCanvasTests)
    set_tests_properties(PuzzleCanvasTests PROPERTIES
            TIMEOUT 30
    )
endif ()
