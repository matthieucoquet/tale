module;
#include <array>
#include <compare>
#include <cstdint>
#include <stdexcept>
#include <vma_includes.hpp>
#include <vulkan/vulkan.h>
export module tale.vulkan.monitor_swapchain;
import vulkan_hpp;
import tale.vulkan.context;
import tale.vulkan.image;

namespace tale::vulkan {
export class Monitor_swapchain {
public:
    static constexpr vk::Format format{vk::Format::eB8G8R8A8Srgb};
    static constexpr uint32_t image_count{3u};

    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::array<vk::Image, image_count> images;
    // std::array<vk::ImageView, image_count> image_views;

    Monitor_swapchain(Context& context);
    Monitor_swapchain(const Monitor_swapchain& other) = delete;
    Monitor_swapchain(Monitor_swapchain&& other) = delete;
    Monitor_swapchain& operator=(const Monitor_swapchain& other) = default;
    Monitor_swapchain& operator=(Monitor_swapchain&& other) = default;
    ~Monitor_swapchain();

private:
    vk::Device device;
};
}

module :private;

namespace tale::vulkan {
Monitor_swapchain::Monitor_swapchain(Context& context):
    device(context.device) {
    constexpr vk::ColorSpaceKHR colorspace{vk::ColorSpaceKHR::eSrgbNonlinear};
    constexpr vk::PresentModeKHR present_mode{vk::PresentModeKHR::eFifo};

    const auto available_formats = context.physical_device.getSurfaceFormatsKHR(context.surface);
    if (std::ranges::find(available_formats, vk::SurfaceFormatKHR{format, colorspace}) == available_formats.end())
        throw std::runtime_error("Failed to find required surface format.");
    const auto available_present_modes = context.physical_device.getSurfacePresentModesKHR(context.surface);
    if (std::ranges::find(available_present_modes, present_mode) == available_present_modes.end())
        throw std::runtime_error("Failed to find required surface present mode.");

    const auto surface_capabilities = context.physical_device.getSurfaceCapabilitiesKHR(context.surface);
    if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = surface_capabilities.currentExtent;
    } else {
        throw std::runtime_error("Surface capabilities extend is set to UINT32_MAX, currently not supported.");
    }
    if (image_count < surface_capabilities.minImageCount && image_count > surface_capabilities.maxImageCount)
        throw std::runtime_error("Doesn't support required image count.");

    swapchain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
        .surface = context.surface,
        .minImageCount = image_count,
        .imageFormat = format,
        .imageColorSpace = colorspace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true
    });

    const auto vec_images = device.getSwapchainImagesKHR(swapchain);
    for (size_t i = 0u; i < image_count; i++) {
        images[i] = vec_images[i];

        // image_views[i] = device.createImageView(vk::ImageViewCreateInfo{
        //     .image = images[i],
        //     .viewType = vk::ImageViewType::e2D,
        //     .format = format,
        //     .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}
        // });
    }
}

Monitor_swapchain::~Monitor_swapchain() {
    if (!swapchain)
        return;
    // for (auto image_view : image_views) {
    //     device.destroyImageView(image_view);
    // }
    device.destroySwapchainKHR(swapchain);
    swapchain = nullptr;
}

}
