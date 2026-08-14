set(KROMA_ENGINE_NAME KromaEngine)
set(KROMA_APP_NAME Kroma)

file(GLOB_RECURSE KROMA_ENGINE_SRC_FILES 
    "Source/Core/*.c"
    "Source/Core/*.h"
    "Source/Graphics/*.c"
    "Source/Graphics/*.h"
    "Source/Scene/*.c"
    "Source/Scene/*.h"
)

file(GLOB_RECURSE KROMA_APP_SRC_FILES 
    "Source/App/*.c"
    "Source/App/*.h"
)

add_library(${KROMA_ENGINE_NAME} SHARED ${KROMA_ENGINE_SRC_FILES})
target_compile_definitions(${KROMA_ENGINE_NAME} PRIVATE KROMA_BUILD_SHARED UMBRACOMPILER_BUILD_SHARED)

add_executable(${KROMA_APP_NAME} ${KROMA_APP_SRC_FILES})


set(KROMA_LIST ${KROMA_ENGINE_NAME} ${KROMA_APP_NAME})

foreach (NAME ${KROMA_LIST})
    target_include_directories(${NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_SOURCE_DIR}/Thirdparty"
        "${CMAKE_SOURCE_DIR}/Thirdparty/CGLM/include"
        "${CMAKE_SOURCE_DIR}/Thirdparty/ShaderCompiler/Include"
        "${CMAKE_SOURCE_DIR}/Thirdparty/CGLTF"
        "$ENV{VULKAN_SDK}/Include"
    )
    if(WIN32)
        target_link_libraries(${NAME} PRIVATE
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
endforeach()


target_link_libraries(${KROMA_ENGINE_NAME} PRIVATE cglm UmbraCompiler SDL3::SDL3)
target_link_libraries(${KROMA_APP_NAME} PRIVATE cglm UmbraCompiler SDL3::SDL3 ${KROMA_ENGINE_NAME})

add_custom_command(TARGET ${KROMA_ENGINE_NAME} POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:${KROMA_ENGINE_NAME}>/Resources
)
