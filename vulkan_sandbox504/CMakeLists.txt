cmake_minimum_required(VERSION 3.15)
include(FetchContent)

project(Vulkan_Sandbox LANGUAGES C CXX)

# Force MSVC to use Release CRT (/MD) and Iterator Debug Level 0 for ALL configs (Debug/Release)
# This is required to successfully link the Release version of shaderc_combined.lib inside a Debug build
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
endif()

# ============================================================================-
# General project options
# ============================================================================-

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# compile flags
set(COMPILE_OPTIONS "")

if(MSVC)
    list(APPEND COMPILE_OPTIONS
        /W4
    )
else()
    list(APPEND COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic
        -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
        -Wdangling-else
        -Wmisleading-indentation
    )

endif()

# Source and Libs dir
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

# ============================================================================
# helpers
# ============================================================================
function(mark_headers_system target)
    # Make the target’s PUBLIC/INTERFACE include dirs system-paths, so the
    # compiler & clang-d tidy shut up when code #includes them.
    if (TARGET "${target}")
        get_target_property(dirs "${target}" INTERFACE_INCLUDE_DIRECTORIES)
        if (dirs)
            target_include_directories("${target}" SYSTEM INTERFACE ${dirs})
        endif()
    endif()
endfunction()

function(disable_warnings_for target)
    # Stop the compiler from emitting any warnings while it is actually
    # compiling this target’s own .cpp/.c files.
    if (TARGET "${target}")
        if (MSVC)
            target_compile_options("${target}" PRIVATE /w)
        else()          # GCC / Clang
            target_compile_options("${target}" PRIVATE -w)
        endif()
    endif()
endfunction()

# ============================================================================
# Libraries
# ============================================================================

# Vulkan + shaderc
add_library(Vulkan_Dummy INTERFACE)
add_library(Vulkan::Vulkan ALIAS Vulkan_Dummy)

# 1. Vulkan-Headers
target_include_directories(Vulkan_Dummy INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/Vulkan-Headers/include)
set(Vulkan_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libs/Vulkan-Headers/include)

# 2. Vulkan-Hpp
target_include_directories(Vulkan_Dummy INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/Vulkan-Hpp)



# 3. Bundled Shaderc
add_library(shaderc_combined STATIC IMPORTED)
set_target_properties(shaderc_combined PROPERTIES
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/lib/shaderc_combined.lib"
    IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/lib/shaderc_combined.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/lib/shaderc_combined.lib"
    IMPORTED_LOCATION_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/lib/shaderc_combined.lib"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/lib/shaderc_combined.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/shaderc/include"
)
add_library(Vulkan::shaderc_combined ALIAS shaderc_combined)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw SOURCE_DIR ${LIBS_DIR}/glfw)
FetchContent_MakeAvailable(glfw)
set_target_properties(glfw uninstall update_mappings PROPERTIES FOLDER Libs/GLFW3)

# GLM
FetchContent_Declare(glm SOURCE_DIR ${LIBS_DIR}/glm)
FetchContent_MakeAvailable(glm)
set_target_properties(glm PROPERTIES FOLDER Libs)

# ImGui
FetchContent_Declare(imgui SOURCE_DIR ${LIBS_DIR}/imgui)
FetchContent_MakeAvailable(imgui)
set_target_properties(imgui PROPERTIES FOLDER Libs)

# SPIRV-Reflect
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
FetchContent_Declare(spirv-reflect-static SOURCE_DIR ${LIBS_DIR}/SPIRV-Reflect)
FetchContent_MakeAvailable(spirv-reflect-static)
set_target_properties(spirv-reflect-static PROPERTIES FOLDER Libs)

# KomiHash
add_library(komihash INTERFACE)
target_include_directories(komihash INTERFACE ${LIBS_DIR}/komihash)

# Equine Engine
FetchContent_Declare(EquineEngine SOURCE_DIR ${LIBS_DIR}/EquineEngine)
FetchContent_MakeAvailable(EquineEngine)
set_target_properties(EquineEngine PROPERTIES FOLDER Libs)

# stb image
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${LIBS_DIR}/stb)

# imgui-filebrowser
add_library(imgui-filebrowser INTERFACE)
target_include_directories(imgui-filebrowser INTERFACE ${LIBS_DIR}/imgui-filebrowser)

# tinyGLTF
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_library(tinyGLTF INTERFACE)
target_include_directories(tinyGLTF INTERFACE ${LIBS_DIR}/tinygltf)

# Json
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${LIBS_DIR}/nlohmann)

# nvml (NVIDIA Management Library) - optional, loaded from dll

# reshade
#add_custom_command(
#    OUTPUT build_dependency.stamp
#    COMMAND ${CMAKE_COMMAND} -E echo "Building reshade solution..."
#   COMMAND msbuild ${LIBS_DIR}/reshade/ReShade.sln /p:Configuration=Release /p:Platform=64-bit
#   COMMAND ${CMAKE_COMMAND} -E touch build_dependency.stamp
#   COMMENT "Building external dependency solution"
#   VERBATIM
#)


# === mark all third-party libs ===============
mark_headers_system(EquineEngine)
mark_headers_system(imgui)
mark_headers_system(glfw)
mark_headers_system(glm)
mark_headers_system(spirv-reflect-static)
mark_headers_system(stb)
mark_headers_system(tinyGLTF)
mark_headers_system(json)
mark_headers_system(imgui-filebrowser)
mark_headers_system(komihash)

# (glm/stb/json/tinyGLTF are header-only → nothing to compile)
disable_warnings_for(imgui)
disable_warnings_for(glfw)
disable_warnings_for(spirv-reflect-static)

# ============================================================================
# Executable
# ============================================================================

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p=${CMAKE_BINARY_DIR}")


file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR} ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} EquineEngine glfw glm imgui spirv-reflect-static komihash imgui-filebrowser stb tinyGLTF json Vulkan::Vulkan Vulkan::shaderc_combined)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})


# Add a custom command to create an empty ini file for reshade
#add_custom_command(
#    TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/$<CONFIG>/Reshade.ini"
#)
