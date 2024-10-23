cmake_minimum_required(VERSION 3.30)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
# find_package(imgui CONFIG REQUIRED)
find_package(spdlog REQUIRED)
# find_package(Stb REQUIRED)
find_package(OpenXR CONFIG REQUIRED)

find_package(unofficial-shaderc CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
# For now I use physx as a submodule to make sure to have the latest version
# I could switch to the physx package from vcpkg to have the unofficial not up to date version
find_package(physx MODULE REQUIRED)

add_library(engine STATIC)
target_sources(engine
    PRIVATE
    vulkan/vma_implementation.cpp)
    # vulkan/stb_implementation.cpp)

target_sources(engine
    PUBLIC
    FILE_SET CXX_MODULES FILES
    app.cpp
    core/scene.cpp
    core/window.cpp
    engine/engine.cpp
    engine/monitor_render_system.cpp
    engine/physics_system.cpp
    engine/shader_system.cpp
    engine/vr_system.cpp
    engine/system.cpp
    vr/vr.cpp
    vr/instance.cpp
    vr/vr_swapchain.cpp
    vr/session.cpp
    vulkan/vulkan.cpp
    vulkan/acceleration_structure.cpp
    vulkan/command_buffer.cpp
    vulkan/context.cpp
    vulkan/monitor_swapchain.cpp
    vulkan/raytracing_pipeline.cpp
    vulkan/renderer.cpp
    vulkan/texture.cpp
    vulkan/vma_buffer.cpp
    vulkan/vma_image.cpp
)
target_compile_features(engine PUBLIC cxx_std_23)

target_link_libraries(engine
    PUBLIC
    glm::glm
#     imgui::imgui
    spdlog::spdlog
    vulkan_module
    PRIVATE
    glfw
    unofficial::shaderc::shaderc
    physx::physx
    OpenXR::headers OpenXR::openxr_loader
)

target_include_directories(engine SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(engine/shader_system.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")

if (MSVC)
    target_compile_options(engine PUBLIC /W4 /WX /permissive- /wd5050)
endif()

add_library(tale::engine ALIAS engine)
