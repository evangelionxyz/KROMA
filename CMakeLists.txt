cmake_minimum_required(VERSION 3.20)
project(KROMA)
find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CGML_STATIC ON)
set(PROGRAM_NAME KROMA)
set(UMBRACOMPILER_BUILD_EXAMPLES OFF)

# Build all artifacts into a central directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory("CGLM")
add_subdirectory("SDL")
add_subdirectory(Umbra-ShaderCompiler)

file(GLOB_RECURSE SRC_FILES "Source/*.c")
add_executable(${PROGRAM_NAME} ${SRC_FILES})

target_include_directories(${PROGRAM_NAME} PRIVATE
    "CGLM/include"
    "Umbra-ShaderCompiler/Include"
    "$ENV{VULKAN_SDK}/Include"
)

target_link_libraries(${PROGRAM_NAME} PRIVATE cglm UmbraCompiler SDL3::SDL3)

if(WIN32)
    target_link_libraries(${PROGRAM_NAME} PRIVATE
        cglm
        UmbraCompiler
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

    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:KROMA>/Resources
    )
endif()
