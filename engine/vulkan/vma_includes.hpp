#pragma once

#include <vulkan/vulkan_hpp_macros.hpp>
// VMA shouldn't load function by itself
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>
