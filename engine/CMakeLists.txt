cmake_minimum_required(VERSION 3.28)

find_package(glfw3 CONFIG REQUIRED)
# find_package(glm CONFIG REQUIRED)
# find_package(imgui CONFIG REQUIRED)
find_package(spdlog REQUIRED)
# find_package(Vulkan REQUIRED)
# find_package(Stb REQUIRED)

add_library(engine STATIC)
target_sources(engine
    PRIVATE
    vulkan/vma_implementation.cpp)
    # vulkan/stb_implementation.cpp)

target_sources(engine
   PUBLIC
   FILE_SET CXX_MODULES FILES
   app.cpp
#    core/scene.cpp
   core/window.cpp
   vulkan/context.cpp   
   vulkan/vulkan.cpp
#    vulkan/command_buffer.cpp
#    vulkan/floor_render.cpp
#    vulkan/sphere_render.cpp
#    vulkan/imgui_render.cpp
#    vulkan/pipeline.cpp
#    vulkan/renderer.cpp
#    vulkan/renderpass.cpp
#    vulkan/shader_module.cpp
#    vulkan/swapchain.cpp
#    vulkan/texture.cpp
#    vulkan/vma_buffer.cpp
#    vulkan/vma_image.cpp
#    systems/input_system.cpp
#    systems/ui_system.cpp
)
target_compile_features(engine PUBLIC cxx_std_23)

target_link_libraries(engine
    PUBLIC
#     glm::glm
#     imgui::imgui
    spdlog::spdlog
    vulkan_module
    PRIVATE
    glfw
)

target_include_directories(engine SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan)

if (MSVC)
    target_compile_options(engine PUBLIC /W4 /WX /permissive-)
endif()

add_library(tale::engine ALIAS engine)
