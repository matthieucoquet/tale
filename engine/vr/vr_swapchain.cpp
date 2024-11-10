module;
#include "vr_common.hpp"
#include <spdlog/spdlog.h>
export module tale.vr.swapchain;
import std;
import vulkan_hpp;
// import tale.vulkan.context;
import tale.vr.instance;

namespace tale::vr {
export class Vr_swapchain {
public:
    static constexpr vk::Format required_color_format = vk::Format::eR8G8B8A8Srgb;
    // static constexpr vk::Format required_depth_format = vk::Format::eR8G8B8A8Srgb;
    xr::Swapchain color_swapchain;
    std::vector<vk::Image> color_images;
    xr::Extent2Di view_extent;

    Vr_swapchain(Instance& instance, xr::Session& session);
    Vr_swapchain(const Vr_swapchain& other) = delete;
    Vr_swapchain(Vr_swapchain&& other) = delete;
    Vr_swapchain& operator=(const Vr_swapchain& other) = delete;
    Vr_swapchain& operator=(Vr_swapchain&& other) = delete;
    ~Vr_swapchain();

    [[nodiscard]] vk::Extent2D vk_view_extent() const {
        return vk::Extent2D{.width = static_cast<uint32_t>(view_extent.width * 2), .height = static_cast<uint32_t>(view_extent.height)};
    }

private:
    std::vector<xr::SwapchainImageVulkanKHR> xr_color_images;
    // xr::Swapchain depth_swapchain;
    // std::vector<xr::SwapchainImageVulkanKHR> xr_depth_images;
    // std::vector<vk::Image> depth_images;
};

}

module :private;

namespace tale::vr {

Vr_swapchain::Vr_swapchain(Instance& instance, xr::Session& session) {
    const std::vector<int64_t> supported_formats = session.enumerateSwapchainFormatsToVector();

    spdlog::debug("Supported Vulkan format in the XR runtime:");
    for (auto format : supported_formats) {
        spdlog::debug("\t{}", vk::to_string(static_cast<vk::Format>(format)));
    }
    if (std::ranges::none_of(supported_formats, [](const auto& format) { return static_cast<int64_t>(required_color_format) == format; })) {
        throw std::runtime_error("Required format not supported by OpenXR runtime.");
    }

    const auto view_configuration_views =
        instance.instance.enumerateViewConfigurationViewsToVector(instance.system_id, xr::ViewConfigurationType::PrimaryStereo);
    xr::ViewConfigurationView view_configuration_view = view_configuration_views[0];
    if (view_configuration_views.size() != 2) {
        throw std::runtime_error("Expected 2 view configuration views.");
    }
    if (view_configuration_views[0].recommendedImageRectHeight != view_configuration_views[1].recommendedImageRectHeight ||
        view_configuration_views[0].recommendedImageRectWidth != view_configuration_views[1].recommendedImageRectWidth) {
        throw std::runtime_error("Expected view configuration views to have the same recommended image rect height.");
    }
    if (view_configuration_views[0].recommendedSwapchainSampleCount != view_configuration_views[1].recommendedSwapchainSampleCount) {
        throw std::runtime_error("Expected view configuration views to have the same recommended swapchain sample count.");
    }
    spdlog::debug("XR runtime view configuration views:");
    for (const auto& view : view_configuration_views) {
        spdlog::debug("\tRecommended: {}x{} {} samples", view.recommendedImageRectWidth, view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount);
    }

    view_extent.height = view_configuration_view.recommendedImageRectHeight;
    view_extent.width = view_configuration_view.recommendedImageRectWidth;
    {
        xr::SwapchainCreateInfo create_info{};
        create_info.createFlags = xr::SwapchainCreateFlagBits::None;
        create_info.usageFlags = xr::SwapchainUsageFlagBits::TransferDst; // We could use UnorderedAccess to directly write to the image, but it might not be
                                                                          // compatible with srgb format
        create_info.format = static_cast<int64_t>(required_color_format);
        create_info.sampleCount = view_configuration_views[0].recommendedSwapchainSampleCount;
        create_info.width = view_configuration_views[0].recommendedImageRectWidth * 2u; // One swapchain of double width
        create_info.height = view_configuration_views[0].recommendedImageRectHeight;
        create_info.faceCount = 1;
        create_info.arraySize = 1;
        create_info.mipCount = 1;
        color_swapchain = session.createSwapchain(create_info);

        xr_color_images = color_swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();
        color_images.reserve(xr_color_images.size());
        for (const auto& image : xr_color_images) {
            color_images.push_back(image.image);
        }
    }

    // {
    //     xr::SwapchainCreateInfo create_info{};
    //     create_info.createFlags = xr::SwapchainCreateFlagBits::None;
    //     create_info.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
    //     create_info.format = static_cast<int64_t>(required_depth_format);
    //     create_info.sampleCount = view_configuration_views[0].recommendedSwapchainSampleCount;
    //     create_info.width = view_configuration_views[0].recommendedImageRectWidth * 2u; // One swapchain of double width
    //     create_info.height = view_configuration_views[0].recommendedImageRectHeight;
    //     create_info.faceCount = 1;
    //     create_info.arraySize = 1;
    //     create_info.mipCount = 1;
    //     depth_swapchain = session.createSwapchain(create_info);

    //     xr_depth_images = depth_swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();
    //     depth_images.reserve(xr_depth_images.size());
    //     for (const auto& image : xr_depth_images) {
    //         depth_images.push_back(image.image);
    //     }
    // }
}

Vr_swapchain::~Vr_swapchain() {
    // if (color_swapchain)
    //     color_swapchain.destroy();
    // depth_swapchain.destroy();
}

}