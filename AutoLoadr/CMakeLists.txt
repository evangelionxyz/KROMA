cmake_minimum_required(VERSION 3.20)

project(AutoLoadr)
set(CMAKE_CXX_STANDARD 26)

file(GLOB_RECURSE SRC_FILES
    Source/*.cpp
    Include/*.hpp
    Include/*.h
)
add_library(AutoLoadr SHARED ${SRC_FILES})
target_include_directories(AutoLoadr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(AutoLoadr PRIVATE AUTOLOADR_SHARED_BUILD)

if (WIN32)
    message("   >> Platform: Windows")
    
    # enable Hot Reload for MSVC compilers if supported.
    if (POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT $<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>)
    endif()
elseif (UNIX AND NOT APPLE)
    # check if we're running in WSL
    execute_process(COMMAND uname -r OUTPUT_VARIABLE WSL_KERNEL_NAME)
    string(FIND "${WSL_KERNEL_NAME}" "Microsoft" WSL_FOUND)
    
    if (WSL_FOUND EQUAL -1)
        message("   >> Platform: WSL")
    else()
        message("   >> Platform: LINUX")
    endif()
endif()

install(TARGETS AutoLoadr
    EXPORT AutoLoadrTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h;*.hpp"
)

install(EXPORT AutoLoadrTargets
    FILE AutoLoadr.cmake
    NAMESPACE AutoLoadr::
    DESTINATION lib/cmake/AutoLoadr
)

# linking
if (WIN32)
elseif (UNIX AND NOT APPLE)
endif()