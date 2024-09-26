module;
#include <spdlog/spdlog.h>
#define XR_USE_GRAPHICS_API_VULKAN
// clang-format off
#include <vulkan/vulkan.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
// clang-format on
export module tale.vr.instance;

namespace tale::vr {
export class Instance {
public:
    Instance();
    Instance(const Instance& other) = delete;
    Instance(Instance&& other) = delete;
    Instance& operator=(const Instance& other) = delete;
    Instance& operator=(Instance&& other) = delete;
    ~Instance();

private:
    xr::Instance instance;
};
}

module :private;

namespace tale::vr {
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

    const xr::ApplicationInfo application_info("Tale", 1, "Tale Engine", 1, xr::Version::current());
    const xr::InstanceCreateInfo instance_create_info(
        xr::InstanceCreateFlagBits::None, application_info, 0, nullptr, static_cast<uint32_t>(required_extensions.size()), required_extensions.data()
    );
    instance = xr::createInstance(instance_create_info);
}

Instance::~Instance() { /*xr::destroyInstance(instance);*/ }
}
