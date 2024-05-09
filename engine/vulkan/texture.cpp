module;
#include <compare>
#include <string_view>
#include <vma_includes.hpp>
export module tale.vulkan.texture;
import vulkan_hpp;
import tale.vulkan.context;
import tale.vulkan.command_buffer;
import tale.vulkan.image;

namespace tale::vulkan {
export class Texture {};

export class Storage_texture {
public:
    Vma_image image;
    vk::ImageView image_view;

    Storage_texture(Context& context, vk::Extent2D extent, vk::CommandBuffer command_buffer);
    Storage_texture(const Storage_texture& other) = delete;
    Storage_texture(Storage_texture&& other) noexcept;
    Storage_texture& operator=(const Storage_texture& other) = delete;
    Storage_texture& operator=(Storage_texture&& other) noexcept;
    ~Storage_texture();

private:
    vk::Device device;
};
}
module :private;

namespace tale::vulkan {

static constexpr vk::Format storage_format = vk::Format::eR8G8B8A8Unorm;

Storage_texture::Storage_texture(Storage_texture&& other) noexcept:
    image(std::move(other.image)),
    image_view(other.image_view),
    device(other.device) {
    other.device = nullptr;
}

Storage_texture& Storage_texture::operator=(Storage_texture&& other) noexcept {
    image = std::move(other.image);
    std::swap(image_view, other.image_view);
    std::swap(device, other.device);
    return *this;
}

Storage_texture::Storage_texture(Context& context, vk::Extent2D extent, vk::CommandBuffer command_buffer):
    image(
        context.device, context.allocator,
        vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = storage_format,
            .extent = {extent.width, extent.height, 1},
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        },
        VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_AUTO}
    ),
    device(context.device) {
    image_view = device.createImageView(vk::ImageViewCreateInfo{
        .image = image.image,
        .viewType = vk::ImageViewType::e2D,
        .format = storage_format,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}
    });
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
        }
    );
}

Storage_texture::~Storage_texture() {
    if (device) {
        device.destroyImageView(image_view);
        device = nullptr;
    }
}

}