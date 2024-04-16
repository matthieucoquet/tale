module;
#include <cassert>
#include <compare>
#include <utility>
#include <vma_includes.hpp>
export module tale.vulkan.image;
import vulkan_hpp;
import tale.vulkan.buffer;

namespace tale::vulkan {

export class Vma_image {
public:
    vk::Image image;

    Vma_image() = default;
    Vma_image(const Vma_image& other) = delete;
    Vma_image(Vma_image&& other) noexcept;
    Vma_image& operator=(const Vma_image& other) = delete;
    Vma_image& operator=(Vma_image&& other) noexcept;
    Vma_image(vk::Device device, VmaAllocator allocator, vk::ImageCreateInfo image_info, VmaAllocationCreateInfo allocation_info);
    ~Vma_image();

private:
    vk::Device device;
    VmaAllocator allocator{};
    VmaAllocation allocation{};
};

export struct Image_from_staged {
    [[nodiscard]] Image_from_staged(
        vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::ImageCreateInfo image_info, const void* data, size_t size
    );
    Vma_image result;
    Vma_buffer staging;
};

}

module :private;

namespace tale::vulkan {
Vma_image::Vma_image(Vma_image&& other) noexcept:
    image(std::move(other.image)),
    device(other.device),
    allocator(other.allocator),
    allocation(other.allocation) {
    other.device = nullptr;
    other.image = nullptr;
    other.allocator = nullptr;
    other.allocation = nullptr;
}

Vma_image& Vma_image::operator=(Vma_image&& other) noexcept {
    std::swap(device, other.device);
    std::swap(image, other.image);
    std::swap(allocator, other.allocator);
    std::swap(allocation, other.allocation);
    return *this;
}

Vma_image::Vma_image(vk::Device device, VmaAllocator allocator, vk::ImageCreateInfo image_info, VmaAllocationCreateInfo allocation_info):
    device(device),
    allocator(allocator) {
    const VkImageCreateInfo& c_image_info = image_info;

    VkImage c_image;
    [[maybe_unused]] VkResult result = vmaCreateImage(allocator, &c_image_info, &allocation_info, &c_image, &allocation, nullptr);
    assert(result == VK_SUCCESS);
    image = c_image;
}

Vma_image::~Vma_image() {
    if (device) {
        device.destroyImage(image);
        vmaFreeMemory(allocator, allocation);
    }
}

Image_from_staged::Image_from_staged(
    vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::ImageCreateInfo image_info, const void* data, size_t size
) {
    staging = Vma_buffer(
        device, allocator, vk::BufferCreateInfo{.size = size, .usage = vk::BufferUsageFlagBits::eTransferSrc},
        VmaAllocationCreateInfo{.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO}
    );
    staging.map();
    staging.copy(data, size);
    staging.unmap();

    image_info.usage |= vk::ImageUsageFlagBits::eTransferDst;
    result = Vma_image(device, allocator, image_info, VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_AUTO});

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
        }
    );
    command_buffer.copyBufferToImage(
        staging.buffer, result.image, vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{.imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1}, .imageExtent = image_info.extent}
    );
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
        vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
        }
    );
}
}