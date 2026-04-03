# Equine Engine
cmake_minimum_required(VERSION 3.14)

project(EquineEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EQUINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE EQUINE_SOURCES ${EQUINE_DIR}/*.cpp ${EQUINE_DIR}/*.h ${EQUINE_DIR}/*.hpp)
source_group(TREE ${EQUINE_DIR} PREFIX "" FILES ${EQUINE_SOURCES})

add_library(EquineEngine STATIC ${EQUINE_SOURCES})

target_include_directories(EquineEngine PUBLIC
    ${EQUINE_DIR}
	  ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS})

target_link_libraries(EquineEngine glfw glm imgui spirv-reflect-static komihash Vulkan::Vulkan Vulkan::shaderc_combined)

# Ensure all translation units use dynamic dispatcher so extension funcs (like vkSetDebugUtilsObjectNameEXT)
# are resolved via function pointers instead of requiring import library symbols.
target_compile_definitions(EquineEngine PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
