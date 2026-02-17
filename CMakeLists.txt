cmake_minimum_required(VERSION 3.20)
project(KROMA)

find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(CGML_STATIC ON)
add_subdirectory("CGLM")

include(CompileShaders.cmake)

file(GLOB_RECURSE SRC_FILES "Source/*.c")
add_executable(KROMA ${SRC_FILES})
add_dependencies(KROMA compile_shaders)

target_include_directories(KROMA PRIVATE
    "SDL3/include"
    "CGLM/include"
    "$ENV{VULKAN_SDK}/Include"
)

target_link_libraries(KROMA PRIVATE
    cglm
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/windows/x64/SDL3.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/windows/x64/SDL3_ttf.lib
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-cd.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-glsld.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-hlsld.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-msld.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-cppd.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-reflectd.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-c.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-glsl.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-hlsl.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-msl.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-cpp.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-reflect.lib>
)

add_custom_command(TARGET KROMA POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/windows/x64/SDL3.dll"
        "$<TARGET_FILE_DIR:KROMA>/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/windows/x64/SDL3_ttf.dll"
        "$<TARGET_FILE_DIR:KROMA>/SDL3_ttf.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "$<TARGET_FILE_DIR:KROMA>/Resources"
)
