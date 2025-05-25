# On macOS, the default Apple Clang compilers don't support the standard library
# parallel algorithms, but the GCC compilers do. So, you need to install GCC from
# Homebrew and use `cmake --toolchain=toolchain.cmake` to use the GCC compilers.
set(CMAKE_C_COMPILER /opt/homebrew/bin/gcc-14)
set(CMAKE_CXX_COMPILER /opt/homebrew/bin/g++-14)
