# Packaging Puzzle for Windows

The release package is self-contained. A target computer does not need Qt or
MinGW installed.

From the project root, run:

```powershell
cmake --preset qt-mingw-release
cmake --build --preset qt-mingw-release
cmake --install build-release
cmake --build --preset qt-mingw-package
```

The unpacked deployment is written to `dist/Puzzle`. The package preset creates
`build-release/Puzzle-0.1.0-windows-x64.zip`, containing the complete portable
application.

Before publishing a release, test the unpacked application on a clean Windows
machine by launching `dist/Puzzle/bin/Puzzle.exe`. Keep the complete `bin`
directory together; the executable depends on the DLLs, plugins, and QML
modules deployed beside it.

The Qt installation and toolchain locations used by the presets are defined in
`CMakePresets.json`. Update the hidden `qt-mingw-base` preset if Qt or Ninja is
moved.
