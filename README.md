# Baba Is You solver

This repo contains the code for a program that can solve levels from the video game *Baba Is You*.

## Limitations

The program has the following limitations. Eventually I'd like to change the code so that it no
longer has these limitations.

1. Currently, the program can only solve one level, The Floatiest Platforms (Mountain-Extra 1).

## How to build and run the code

### macOS

1. Install [CMake](https://cmake.org/) and [Homebrew](https://brew.sh/).
2. In the terminal, install dependencies: `brew install gcc` and `brew install tbb`
3. Clone this repo and navigate to the `src/` directory.
4. Generate CMake files: `cmake -B build --toolchain=toolchain.cmake`
5. Build the code: `cmake --build build`
6. Run the binary: `build/BabaSolver`

### Windows

1. Install [CMake](https://cmake.org/) and [Visual Studio](https://visualstudio.microsoft.com/).
2. Clone this repo and navigate to the `src/` directory.
3. Generate CMake files: `cmake -B build`
4. Build and run the code in Visual Studio through the generated .sln file.

## How the algorithm works

TODO

## FAQ

TODO
