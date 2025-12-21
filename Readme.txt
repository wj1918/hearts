CMake is a cross-platform build system generator. It doesn't compile code directly—instead, it generates native build files for your platform:

  - Windows: Visual Studio solutions (.sln), MSBuild files
  - Linux/macOS: Makefiles
  - Any platform: Ninja build files

  Key benefits:
  - Write one CMakeLists.txt, build on any platform
  - Handles compiler detection, dependencies, and linking
  - Supports out-of-source builds (keeps source tree clean)

  Basic workflow:
  mkdir build && cd build
  cmake ..                    # Generate build files
  cmake --build .             # Compile
  cmake --build . --config Release  # Release build

  In your project, CMakeLists.txt defines the hearts executable and hearts_lib static library, finds the threading library, and sets compiler flags.
