set(KROMA_TEST_NAME KromaTest)

file(GLOB_RECURSE KROMA_TEST_SRC_FILES "Source/*.c" "Source/*.h")
add_executable(${KROMA_TEST_NAME} ${KROMA_TEST_SRC_FILES})

target_include_directories(${KROMA_TEST_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${CMAKE_SOURCE_DIR}/Thirdparty"
    "${CMAKE_SOURCE_DIR}/Thirdparty/CGLM/include"
    "${CMAKE_SOURCE_DIR}/Thirdparty/ShaderCompiler/Include"
    "${CMAKE_SOURCE_DIR}/Thirdparty/CGLTF"
    "$ENV{VULKAN_SDK}/Include"
)

target_link_libraries(${KROMA_TEST_NAME} PRIVATE cglm UmbraCompiler SDL3::SDL3)

if(WIN32)
    target_link_libraries(${KROMA_TEST_NAME} PRIVATE
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
endif()
