module;
#include <spdlog/spdlog.h>
#include <vma_includes.hpp>
export module tale.vulkan.context;
import std;
import vulkan_hpp;
import tale.window;

namespace tale::vulkan {
export class Context {
public:
    vk::Instance instance;
    vk::SurfaceKHR surface; // Should be optional or owned by window
    vk::Device device;
    vk::PhysicalDevice physical_device;
    vk::CommandPool command_pool;
    uint32_t queue_family = 0u;
    vk::Queue queue;
    VmaAllocator allocator;
    vk::DescriptorPool descriptor_pool;

    Context(Window& window);
    Context(const Context& other) = delete;
    Context(Context&& other) = delete;
    Context& operator=(const Context& other) = delete;
    Context& operator=(Context&& other) = delete;
    ~Context();

private:
    void init_instance(const Window& window);
    void init_device();
    void init_allocator();
    void init_descriptor_pool();
};

}

module :private;

constexpr bool use_validation_layers = true;

namespace tale::vulkan {

Context::Context(Window& window) {
    init_instance(window);
    surface = window.create_surface(instance);
    init_device();
    init_allocator();
    init_descriptor_pool();
}

Context::~Context() {
    vmaDestroyAllocator(allocator);
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyCommandPool(command_pool);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroy();
}

void Context::init_instance(const Window& window) {
    const auto required_extensions = window.required_extensions();

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    spdlog::debug("Instance layers:");
    for (const auto& property : vk::enumerateInstanceLayerProperties()) {
        spdlog::debug("\t{}", std::string_view(property.layerName));
    }
    spdlog::debug("Instance extensions:");
    for (const auto& property : vk::enumerateInstanceExtensionProperties()) {
        spdlog::debug("\t{}", std::string_view(property.extensionName));
    }
    spdlog::debug("Required instance extensions:");
    for (const auto& required_extension : required_extensions) {
        spdlog::debug("\t{}", required_extension);
    }

    constexpr std::array required_layers{"VK_LAYER_KHRONOS_validation"};

    vk::ApplicationInfo app_info{.pApplicationName = "tale", .apiVersion = VK_API_VERSION_1_3};
    instance = vk::createInstance(vk::InstanceCreateInfo{
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(use_validation_layers ? required_layers.size() : 0u),
        .ppEnabledLayerNames = use_validation_layers ? required_layers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()
    });
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void Context::init_device() {
    const std::array required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    const std::vector potential_physical_devices = instance.enumeratePhysicalDevices();

    // Loop over physical devices and find the best one
    const vk::PhysicalDevice* selected_physical_device = nullptr;
    uint32_t selected_queue_family = 0u;
    spdlog::info("Available GPUs:");
    for (const auto& potential_physical_device : potential_physical_devices) {
        const auto properties = potential_physical_device.getProperties();
        spdlog::info("\t{}", std::string_view(properties.deviceName));

        // Check queue family properties
        const auto queue_family_properties = potential_physical_device.getQueueFamilyProperties();
        uint32_t physical_queue_family = 0u;
        for (const auto& property : queue_family_properties) {
            auto flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
            if ((property.queueFlags & flags) == flags) {
                if (potential_physical_device.getSurfaceSupportKHR(queue_family, surface)) {
                    break;
                }
            }
            physical_queue_family++;
        }
        if (physical_queue_family == queue_family_properties.size()) {
            spdlog::debug("\tNo suitable queue family found.");
            continue;
        }

        // Check extensions availability
        const auto available_extensions = potential_physical_device.enumerateDeviceExtensionProperties();
        if (std::ranges::any_of(required_device_extensions, [&available_extensions](const char* required_extension_name) {
                return std::ranges::all_of(available_extensions, [required_extension_name](const VkExtensionProperties& available) {
                    return strcmp(available.extensionName, required_extension_name) != 0;
                });
            })) {
            spdlog::debug("\tRequired extension not available.");
            continue;
        }

        // Checking features
        {
            auto vulkan_12_features = vk::PhysicalDeviceVulkan12Features();
            auto sync_features = vk::PhysicalDeviceSynchronization2FeaturesKHR{.pNext = &vulkan_12_features};
            auto as_features = vk::PhysicalDeviceAccelerationStructureFeaturesKHR{.pNext = &sync_features};
            auto pipeline_features = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{.pNext = &as_features};
            auto features = vk::PhysicalDeviceFeatures2{.pNext = &pipeline_features};
            potential_physical_device.getFeatures2(&features);

            if (!pipeline_features.rayTracingPipeline || !pipeline_features.rayTraversalPrimitiveCulling)
                continue;
            if (!as_features.accelerationStructure)
                continue;
            if (!sync_features.synchronization2)
                continue;
            if (!vulkan_12_features.bufferDeviceAddress || !vulkan_12_features.uniformBufferStandardLayout || !vulkan_12_features.scalarBlockLayout /*||
                !vulkan_12_features.uniformAndStorageBuffer8BitAccess*/)
                continue;
        }

        // Take discrete GPU if there's one, otherwise just take the first GPU
        if (!selected_physical_device || properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            selected_physical_device = &potential_physical_device;
            selected_queue_family = physical_queue_family;
        }
    }
    if (!selected_physical_device) {
        throw std::runtime_error("Did not find a suitable GPU.");
    }
    physical_device = *selected_physical_device;
    queue_family = selected_queue_family;

    {
        const auto properties = physical_device.getProperties();
        spdlog::info("Selected GPU: {}", std::string_view(properties.deviceName));
        const auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
        spdlog::debug("Device extensions:");
        for (const auto& property : available_extensions) {
            spdlog::debug("\t{}", std::string_view(property.extensionName));
        }
    }

    vk::PhysicalDeviceVulkan12Features vulkan_12_features{
        .scalarBlockLayout = true,
        .uniformBufferStandardLayout = true,
        .bufferDeviceAddress = true,
    };
    vk::PhysicalDeviceSynchronization2FeaturesKHR sync_features{.pNext = &vulkan_12_features, .synchronization2 = true};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR raytracing_as_features{.pNext = &sync_features, .accelerationStructure = true};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_pileline_features{
        .pNext = &raytracing_as_features,
        .rayTracingPipeline = true,
        .rayTraversalPrimitiveCulling = true // To skip triangles in ray pipeline, not sure if useful
    };
    vk::PhysicalDeviceFeatures2 device_features{
        .pNext = &raytracing_pileline_features,
    };
    const float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info{.queueFamilyIndex = queue_family, .queueCount = 1u, .pQueuePriorities = &queue_priority};
    device = physical_device.createDevice(vk::DeviceCreateInfo{
        .pNext = &device_features,
        .queueCreateInfoCount = 1u,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size()),
        .ppEnabledExtensionNames = required_device_extensions.data()
    });
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    queue = device.getQueue(queue_family, 0u);
    command_pool =
        device.createCommandPool(vk::CommandPoolCreateInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queue_family});
}

void Context::init_allocator() {
    // We need to tell VMA the vulkan address from the dynamic dispatcher
    VmaVulkanFunctions vulkan_functions{
        .vkGetPhysicalDeviceProperties = vk::defaultDispatchLoaderDynamic.vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vk::defaultDispatchLoaderDynamic.vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vk::defaultDispatchLoaderDynamic.vkAllocateMemory,
        .vkFreeMemory = vk::defaultDispatchLoaderDynamic.vkFreeMemory,
        .vkMapMemory = vk::defaultDispatchLoaderDynamic.vkMapMemory,
        .vkUnmapMemory = vk::defaultDispatchLoaderDynamic.vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vk::defaultDispatchLoaderDynamic.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vk::defaultDispatchLoaderDynamic.vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vk::defaultDispatchLoaderDynamic.vkBindBufferMemory,
        .vkBindImageMemory = vk::defaultDispatchLoaderDynamic.vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vk::defaultDispatchLoaderDynamic.vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vk::defaultDispatchLoaderDynamic.vkGetImageMemoryRequirements,
        .vkCreateBuffer = vk::defaultDispatchLoaderDynamic.vkCreateBuffer,
        .vkDestroyBuffer = vk::defaultDispatchLoaderDynamic.vkDestroyBuffer,
        .vkCreateImage = vk::defaultDispatchLoaderDynamic.vkCreateImage,
        .vkDestroyImage = vk::defaultDispatchLoaderDynamic.vkDestroyImage,
        .vkCmdCopyBuffer = vk::defaultDispatchLoaderDynamic.vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vk::defaultDispatchLoaderDynamic.vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = vk::defaultDispatchLoaderDynamic.vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR = vk::defaultDispatchLoaderDynamic.vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR = vk::defaultDispatchLoaderDynamic.vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vk::defaultDispatchLoaderDynamic.vkGetPhysicalDeviceMemoryProperties2KHR
    };

    VmaAllocatorCreateInfo allocator_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vulkan_functions,
        .instance = instance
    };
    vmaCreateAllocator(&allocator_info, &allocator);
}

void Context::init_descriptor_pool() {
    constexpr uint32_t max_frames_in_flight = 10u;
    std::array pool_sizes{
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = max_frames_in_flight},
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = max_frames_in_flight},
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = max_frames_in_flight * 4}
    };
    descriptor_pool = device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
        .maxSets = max_frames_in_flight, .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()), .pPoolSizes = pool_sizes.data()
    });
}
}