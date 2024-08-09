module;
#include <cassert>
#include <compare>
#include <cstring>
#include <utility>
#include <vma_includes.hpp>
export module tale.vulkan.buffer;
import vulkan_hpp;

namespace tale::vulkan {

export class Vma_buffer {
public:
    vk::Buffer buffer;
    void* mapped = nullptr;

    Vma_buffer() = default;
    Vma_buffer(const Vma_buffer& other) = delete;
    Vma_buffer(Vma_buffer&& other) noexcept;
    Vma_buffer& operator=(const Vma_buffer& other) = delete;
    Vma_buffer& operator=(Vma_buffer&& other) noexcept;
    Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaAllocationCreateInfo allocation_info);
    ~Vma_buffer();

    void copy(const void* data, size_t size) { std::memcpy(mapped, data, size); }
    void flush() { vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE); }
    void* map() {
        vmaMapMemory(allocator, allocation, &mapped);
        return mapped;
    }
    void unmap() {
        vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }
    void free() {
        if (device) {
            device.destroyBuffer(buffer);
            vmaFreeMemory(allocator, allocation);
            device = nullptr;
        }
    }

private:
    vk::Device device = nullptr;
    VmaAllocator allocator;
    VmaAllocation allocation{};
};

export struct Buffer_from_staged {
    [[nodiscard]] Buffer_from_staged(
        vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data
    );
    Vma_buffer result;
    Vma_buffer staging;
};

}

module :private;

namespace tale::vulkan {
Vma_buffer::Vma_buffer(Vma_buffer&& other) noexcept:
    buffer(std::move(other.buffer)),
    device(other.device),
    allocator(other.allocator),
    allocation(other.allocation),
    mapped(other.mapped) {
    other.device = nullptr;
    other.buffer = nullptr;
    other.allocator = nullptr;
    other.allocation = nullptr;
    other.mapped = nullptr;
}

Vma_buffer& Vma_buffer::operator=(Vma_buffer&& other) noexcept {
    std::swap(device, other.device);
    std::swap(buffer, other.buffer);
    std::swap(allocator, other.allocator);
    std::swap(allocation, other.allocation);
    std::swap(mapped, other.mapped);
    return *this;
}

Vma_buffer::Vma_buffer(vk::Device device, VmaAllocator allocator, vk::BufferCreateInfo buffer_info, VmaAllocationCreateInfo allocation_info):
    device(device),
    allocator(allocator) {
    const VkBufferCreateInfo& c_buffer_info = buffer_info;
    VkBuffer c_buffer;
    [[maybe_unused]] VkResult result = vmaCreateBuffer(allocator, &c_buffer_info, &allocation_info, &c_buffer, &allocation, nullptr);
    assert(result == VK_SUCCESS);
    buffer = c_buffer;
    if (allocation_info.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        VmaAllocationInfo info{};
        vmaGetAllocationInfo(allocator, allocation, &info);
        mapped = info.pMappedData;
    }
}

Vma_buffer::~Vma_buffer() { free(); }

Buffer_from_staged::Buffer_from_staged(
    vk::Device device, VmaAllocator allocator, vk::CommandBuffer command_buffer, vk::BufferCreateInfo buffer_info, const void* data
) {
    staging = Vma_buffer(
        device, allocator, vk::BufferCreateInfo{.size = buffer_info.size, .usage = vk::BufferUsageFlagBits::eTransferSrc},
        VmaAllocationCreateInfo{.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO}
    );
    staging.map();
    staging.copy(data, buffer_info.size);
    staging.unmap();

    buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    result = Vma_buffer(device, allocator, buffer_info, VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE});

    command_buffer.copyBuffer(staging.buffer, result.buffer, vk::BufferCopy{.srcOffset = 0, .dstOffset = 0, .size = buffer_info.size});
}
}