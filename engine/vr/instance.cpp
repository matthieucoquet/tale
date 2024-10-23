module;
#include "vr_common.hpp"
#include <spdlog/spdlog.h>
export module tale.vr.instance;
import std;

namespace tale::vr {
export class Instance {
public:
    xr::Instance instance;
    xr::SystemId system_id;
    xr::GraphicsBindingVulkanKHR graphic_binding;

    Instance();
    Instance(const Instance& other) = delete;
    Instance(Instance&& other) = delete;
    Instance& operator=(const Instance& other) = delete;
    Instance& operator=(Instance&& other) = delete;
    ~Instance();

    [[nodiscard]] VkInstance create_vulkan_instance(const VkInstanceCreateInfo& create_info, PFN_vkGetInstanceProcAddr instance_proc_addr) const;
    [[nodiscard]] VkPhysicalDevice get_vulkan_physical_device(VkInstance vk_instance) const;
    [[nodiscard]] VkDevice
    create_vulkan_device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo create_info, PFN_vkGetInstanceProcAddr instance_proc_addr) const;

    void set_graphics_binding(VkInstance vk_instance, VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family);

private:
    xr::DispatchLoaderDynamic dispatch_loader_dynamic;
    xr::DebugUtilsMessengerEXT debug_utils_messenger;
};
}

module :private;

namespace tale::vr {

XrBool32 debug_callback(
    XrDebugUtilsMessageSeverityFlagsEXT /*message_severity*/, XrDebugUtilsMessageTypeFlagsEXT /*message_type*/,
    const XrDebugUtilsMessengerCallbackDataEXT* callback_data, void* /*user_data*/
) {
    spdlog::info(callback_data->message);
    return XrBool32();
}

Instance::Instance() {
    std::vector<const char*> required_extensions{XR_EXT_DEBUG_UTILS_EXTENSION_NAME, XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};

    {
        spdlog::debug("OpenXR API layers:");
        const auto properties = xr::enumerateApiLayerPropertiesToVector(xr::DispatchLoaderStatic());
        for (const auto& property : properties) {
            spdlog::debug("\t{}", property.layerName);
        }
    }
    {
        spdlog::debug("Available OpenXR instance extensions:\n");
        const auto properties = xr::enumerateInstanceExtensionPropertiesToVector(nullptr);
        for (const auto& property : properties) {
            spdlog::debug("\t{}", property.extensionName);
        }
    }

    // Initialize instance
    const xr::ApplicationInfo application_info("Tale", 1, "Tale Engine", 1, xr::Version::current());
    const xr::InstanceCreateInfo instance_create_info(
        xr::InstanceCreateFlagBits::None, application_info, 0, nullptr, static_cast<uint32_t>(required_extensions.size()), required_extensions.data()
    );
    instance = xr::createInstance(instance_create_info);

    const auto instance_properties = instance.getInstanceProperties();
    spdlog::info("OpenXR Runtime: {}", instance_properties.runtimeName);

    // Initialize the debug messenger
    xr::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info(
        xr::DebugUtilsMessageSeverityFlagBitsEXT::Error | xr::DebugUtilsMessageSeverityFlagBitsEXT::Warning | xr::DebugUtilsMessageSeverityFlagBitsEXT::Info,
        xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits, (PFN_xrDebugUtilsMessengerCallbackEXT)debug_callback, nullptr
    );
    dispatch_loader_dynamic = xr::DispatchLoaderDynamic(instance);
    dispatch_loader_dynamic.populateFully();
    instance.createDebugUtilsMessengerEXT(debug_utils_create_info, debug_utils_messenger, dispatch_loader_dynamic);

    system_id = instance.getSystem(xr::SystemGetInfo(xr::FormFactor::HeadMountedDisplay));
    const auto system_properties = instance.getSystemProperties(system_id);
    spdlog::info("Device: {}", system_properties.systemName);

    const xr::GraphicsRequirementsVulkanKHR vulkan_requirements = instance.getVulkanGraphicsRequirements2KHR(system_id, dispatch_loader_dynamic);
    if (vulkan_requirements.minApiVersionSupported.major() == 1 && vulkan_requirements.minApiVersionSupported.minor() > 3) {
        throw std::runtime_error("Vulkan version is too old for OpenXR.");
    }

    const auto view_configurations = instance.enumerateViewConfigurationsToVector(system_id);
    if (std::ranges::none_of(view_configurations, [](const auto& config) { return config == xr::ViewConfigurationType::PrimaryStereo; })) {
        throw std::runtime_error("Failed to find a stereo HMD.");
    }
}

Instance::~Instance() {
    if (instance) {
        instance.destroy();
        debug_utils_messenger.destroy(dispatch_loader_dynamic);
    }
}

VkInstance Instance::create_vulkan_instance(const VkInstanceCreateInfo& create_info, PFN_vkGetInstanceProcAddr instance_proc_addr) const {
    const xr::VulkanInstanceCreateInfoKHR instance_create_info(system_id, xr::VulkanInstanceCreateFlagBitsKHR::None, instance_proc_addr, &create_info, nullptr);
    VkResult result;
    VkInstance vk_instance;
    instance.createVulkanInstanceKHR(instance_create_info, &vk_instance, &result, dispatch_loader_dynamic);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not create Vulkan instance.");
    }
    return vk_instance;
}

VkPhysicalDevice Instance::get_vulkan_physical_device(VkInstance vk_instance) const {
    VkPhysicalDevice physical_device;
    instance.getVulkanGraphicsDevice2KHR(xr::VulkanGraphicsDeviceGetInfoKHR(system_id, vk_instance), &physical_device, dispatch_loader_dynamic);
    return physical_device;
}

VkDevice
Instance::create_vulkan_device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo create_info, PFN_vkGetInstanceProcAddr instance_proc_addr) const {
    const xr::VulkanDeviceCreateInfoKHR device_create_info(
        system_id, xr::VulkanDeviceCreateFlagBitsKHR::None, instance_proc_addr, physical_device, &create_info, nullptr
    );

    VkResult result;
    VkDevice device;
    instance.createVulkanDeviceKHR(device_create_info, &device, &result, dispatch_loader_dynamic);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not create Vulkan instance.");
    }
    return device;
}

void Instance::set_graphics_binding(VkInstance vk_instance, VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family) {
    graphic_binding = xr::GraphicsBindingVulkanKHR(vk_instance, physical_device, device, queue_family, 0u);
}

}
