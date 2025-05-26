# CMakeLists.txt : CMake project for BabaSolver

cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("BabaSolver")

add_executable (BabaSolver "Main.cpp" "GameState.h" "Solver.h" "GameState.cpp" "Solver.cpp")

# In GCC, the TBB library is required for the standard library parallel execution policy to
# actually run in parallel.
find_package(TBB REQUIRED)
target_link_libraries(BabaSolver TBB::tbb)

set_property(TARGET BabaSolver PROPERTY CXX_STANDARD 23)

# If the build type is not specified, use the "Release" build type.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# TODO: Add tests and install targets if needed.
