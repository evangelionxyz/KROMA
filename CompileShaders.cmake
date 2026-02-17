set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders")

file(GLOB_RECURSE VERTEX_SHADERS "${SHADER_SOURCE_DIR}/*.vert")
file(GLOB_RECURSE FRAGMENT_SHADERS "${SHADER_SOURCE_DIR}/*.frag")
file(GLOB_RECURSE COMPUTE_SHADERS "${SHADER_SOURCE_DIR}/*.comp")

set(ALL_SHADERS
    ${VERTEX_SHADERS}
    ${FRAGMENT_SHADERS}
    ${COMPUTE_SHADERS}
)

set(SHADER_SPV_OUTPUTS)

foreach(SHADER_SOURCE IN LISTS ALL_SHADERS)
    get_filename_component(SHADER_EXT "${SHADER_SOURCE}" EXT)

    if(SHADER_EXT STREQUAL ".vert")
        set(SHADER_STAGE vert)
    elseif(SHADER_EXT STREQUAL ".frag")
        set(SHADER_STAGE frag)
    elseif(SHADER_EXT STREQUAL ".comp")
        set(SHADER_STAGE comp)
    else()
        continue()
    endif()

    set(SHADER_SPV "${SHADER_SOURCE}.spv")
    list(APPEND SHADER_SPV_OUTPUTS "${SHADER_SPV}")

    add_custom_command(
        OUTPUT "${SHADER_SPV}"
        COMMAND "${GLSLC_EXECUTABLE}" -fshader-stage=${SHADER_STAGE} "${SHADER_SOURCE}" -o "${SHADER_SPV}"
        DEPENDS "${SHADER_SOURCE}"
        COMMENT "Compiling shader ${SHADER_SOURCE}"
        VERBATIM
    )
endforeach()

add_custom_target(compile_shaders DEPENDS ${SHADER_SPV_OUTPUTS})