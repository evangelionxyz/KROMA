cmake_minimum_required(VERSION 3.20)

project(KROMA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CGML_STATIC ON)
set(UMBRACOMPILER_BUILD_EXAMPLES OFF)
set(AUTOLOADR_SHARED_BUILD ON)

# Build all artifacts into a central directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Third-Parrties
add_subdirectory("Thirdparty/CGLM")
add_subdirectory("Thirdparty/SDL3")
add_subdirectory(Thirdparty/ShaderCompiler)

# Programs/Libraries
add_subdirectory(Kroma)
add_subdirectory(Tests)
