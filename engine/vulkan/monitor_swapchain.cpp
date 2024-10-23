module;
#include <vma_includes.hpp>
export module tale.vulkan.monitor_swapchain;
import std;
import vulkan_hpp;
import tale.vulkan.context;
import tale.vulkan.image;

namespace tale::vulkan {
export class Monitor_swapchain {
public:
    // static constexpr vk::Format format{vk::Format::eB8G8R8A8Unorm};
    static constexpr vk::Format format{vk::Format::eB8G8R8A8Srgb};
    static constexpr uint32_t image_count{3u};

    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::array<vk::Image, image_count> images;

    Monitor_swapchain(Context& context, size_t size_command_buffers);
    Monitor_swapchain(const Monitor_swapchain& other) = delete;
    Monitor_swapchain(Monitor_swapchain&& other) = delete;
    Monitor_swapchain& operator=(const Monitor_swapchain& other) = default;
    Monitor_swapchain& operator=(Monitor_swapchain&& other) = default;
    ~Monitor_swapchain();

    void copy_image(vk::CommandBuffer command_buffer, vk::Image source_image, size_t command_pool_id, vk::Extent2D source_extent);
    void present(vk::CommandBuffer& command_buffer, vk::Fence fence, size_t command_pool_id);

private:
    vk::Device device;
    vk::Queue queue;
    std::vector<vk::Semaphore> semaphore_available;
    std::vector<vk::Semaphore> semaphore_finished;
    uint32_t next_image_id{};

    void create_synchronization(size_t size_command_buffers);
};
}

module :private;

namespace tale::vulkan {
Monitor_swapchain::Monitor_swapchain(Context& context, size_t size_command_buffers):
    device(context.device),
    queue(context.queue) {
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
    }

    create_synchronization(size_command_buffers);
}

void Monitor_swapchain::create_synchronization(size_t size_command_buffers) {
    semaphore_available.reserve(size_command_buffers);
    semaphore_finished.reserve(size_command_buffers);
    for (size_t i = 0; i < size_command_buffers; i++) {
        semaphore_available.push_back(device.createSemaphore({}));
        semaphore_finished.push_back(device.createSemaphore({}));
    }
}

Monitor_swapchain::~Monitor_swapchain() {
    if (!swapchain)
        return;
    device.destroySwapchainKHR(swapchain);

    for (size_t i = 0; i < semaphore_available.size(); i++) {
        device.destroySemaphore(semaphore_available[i]);
        device.destroySemaphore(semaphore_finished[i]);
    }

    swapchain = nullptr;
}

void Monitor_swapchain::copy_image(vk::CommandBuffer command_buffer, vk::Image source_image, size_t command_pool_id, vk::Extent2D source_extent) {
    auto [result, framebuffer_id] = device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore_available[command_pool_id], {});

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }
    next_image_id = framebuffer_id;

    {
        std::array memory_barriers{
            vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
                .srcAccessMask = {},
                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .image = images[next_image_id],
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
            },
            vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .image = source_image,
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
            }
        };
        command_buffer.pipelineBarrier2(
            vk::DependencyInfo{.imageMemoryBarrierCount = static_cast<uint32_t>(memory_barriers.size()), .pImageMemoryBarriers = memory_barriers.data()}
        );
    }

    command_buffer.blitImage(
        source_image, vk::ImageLayout::eTransferSrcOptimal, images[next_image_id], vk::ImageLayout::eTransferDstOptimal,
        vk::ImageBlit{
            .srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0u, .baseArrayLayer = 0u, .layerCount = 1u},
            .srcOffsets =
                std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<int32_t>(source_extent.width), static_cast<int32_t>(source_extent.height), 1}},
            .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0u, .baseArrayLayer = 0u, .layerCount = 1u},
            .dstOffsets = std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}}
        },
        vk::Filter::eLinear
    );

    {
        std::array memory_barriers{
            vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
                .dstAccessMask = {},
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::ePresentSrcKHR,
                .image = images[next_image_id],
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
            },
            vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
                .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
                .newLayout = vk::ImageLayout::eGeneral,
                .image = source_image,
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
            }
        };
        command_buffer.pipelineBarrier2(
            vk::DependencyInfo{.imageMemoryBarrierCount = static_cast<uint32_t>(memory_barriers.size()), .pImageMemoryBarriers = memory_barriers.data()}
        );
    }
}

void Monitor_swapchain::present(vk::CommandBuffer& command_buffer, vk::Fence fence, size_t command_pool_id) {
    vk::SemaphoreSubmitInfo wait_semaphore_submit_info{.semaphore = semaphore_available[command_pool_id], .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe};
    vk::SemaphoreSubmitInfo signal_semaphore_submit_info{
        .semaphore = semaphore_finished[command_pool_id], .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe
    };
    vk::CommandBufferSubmitInfo command_buffer_submit_info{.commandBuffer = command_buffer};
    queue.submit2(
        vk::SubmitInfo2{
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &wait_semaphore_submit_info,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &command_buffer_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_semaphore_submit_info
        },
        fence
    );
    const auto present_result = queue.presentKHR(vk::PresentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &semaphore_finished[command_pool_id],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &next_image_id
    });
    if (present_result == vk::Result::eErrorOutOfDateKHR) {
        throw std::runtime_error("presentKHR returned eErrorOutOfDateKHR");
    }
    //[[maybe_unused]] auto result = device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    // queue.waitIdle();
}

}
