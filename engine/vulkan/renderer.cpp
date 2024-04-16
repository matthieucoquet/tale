
module;
#include <glm/glm.hpp>
#include <stdexcept>
#include <vma_includes.hpp>
export module tale.vulkan.renderer;
import vulkan_hpp;
import tale.scene;
import tale.vulkan.context;
import tale.vulkan.monitor_swapchain;
import tale.vulkan.buffer;
import tale.vulkan.command_buffer;
import tale.vulkan.image;

namespace tale::vulkan {

struct Per_frame {
    Vma_image storage_image;
    vk::ImageView storage_image_view;
};

export class Renderer {
public:
    Renderer(Context& context, size_t size_command_buffers);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;
    ~Renderer();

    void reset_swapchain(Context& context);

private:
    vk::Device device;
    Monitor_swapchain swapchain;
    std::vector<Per_frame> per_frame;

    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, size_t command_pool_size);
};
}

module :private;

namespace tale::vulkan {
Renderer::Renderer(Context& context, size_t /*size_command_buffers*/):
    device(context.device),
    swapchain(context) {}

Renderer::~Renderer() { device.waitIdle(); }

void Renderer::reset_swapchain(Context& context) {
    device.waitIdle();
    // We want to call the destructor before the constructor
    swapchain.~Monitor_swapchain();
    [[maybe_unused]] Monitor_swapchain* s = new (&swapchain) Monitor_swapchain(context);
}

void Renderer::create_per_frame_data(Context& context, Scene& /*scene*/, vk::Extent2D extent, size_t command_pool_size) {
    per_frame.reserve(command_pool_size);
    constexpr vk::Format storage_format = vk::Format::eR8G8B8A8Unorm;
    One_time_command_buffer command_buffer(device, context.command_pool, context.queue);
    for (size_t i = 0u; i < command_pool_size; i++) {
        Vma_image image(
            device, context.allocator,
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
        );
        auto image_view = device.createImageView(vk::ImageViewCreateInfo{
            .image = image.image,
            .viewType = vk::ImageViewType::e2D,
            .format = storage_format,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}
        });
        command_buffer.command_buffer.pipelineBarrier(
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

        per_frame.push_back(Per_frame{.storage_image = std::move(image), .storage_image_view = image_view});
    }
}
}