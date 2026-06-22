# CMakeLists.txt : CMake project for BabaSolver

cmake_minimum_required (VERSION 3.20)

project("BabaSolver")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(BabaSolver "Main.cpp" "Solver.cpp" "Summarizer.cpp" "GameState.cpp" "GameStateMtn6.cpp" "GameStateMtnE1.cpp")

if(NOT MSVC)
  # In GCC, the TBB library is required for the standard library parallel execution policy to
  # actually run in parallel.
  find_package(TBB REQUIRED STATIC)
  target_link_libraries(BabaSolver TBB::tbb)
endif()

# Set a default build type if none was specified.
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Optional: Provide a drop-down menu of options for GUI tools like cmake-gui.
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set up Google Test.
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723  # release 1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(BabaSolverTest "SolverTest.cpp" "Solver.cpp" "GameState.cpp")
target_link_libraries(BabaSolverTest GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(BabaSolverTest)
