cmake_minimum_required(VERSION 3.28)

project (tale)

# Vulkan C++ module
set(vulkan_hpp_path ${PROJECT_SOURCE_DIR}/externals/Vulkan-Hpp)
add_library(vulkan_module)
target_sources(vulkan_module
    PRIVATE
    FILE_SET CXX_MODULES
    FILES ${vulkan_hpp_path}/vulkan/vulkan.cppm
)

target_compile_features(vulkan_module PUBLIC cxx_std_23)
target_include_directories(vulkan_module SYSTEM PUBLIC ${vulkan_hpp_path} ${vulkan_hpp_path}/Vulkan-Headers/include)
target_compile_definitions(vulkan_module PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_compile_definitions(vulkan_module PUBLIC VK_NO_PROTOTYPES)
target_compile_definitions(vulkan_module PUBLIC VULKAN_HPP_DYNAMIC_LOADER_TOOL)
target_compile_definitions(vulkan_module PUBLIC VULKAN_HPP_NO_CONSTRUCTORS)
target_compile_definitions(vulkan_module PUBLIC VULKAN_HPP_NO_SMART_HANDLE)


add_subdirectory(engine)
add_subdirectory(app)