
module;
#include <glm/glm.hpp>
#include <stdexcept>
#include <vma_includes.hpp>
export module tale.vulkan.renderer;
import std;
import vulkan_hpp;
import tale.scene;
import tale.vulkan.context;
import tale.vulkan.monitor_swapchain;
import tale.vulkan.buffer;
import tale.vulkan.command_buffer;
import tale.vulkan.image;
import tale.vulkan.texture;
import tale.vulkan.raytracing_pipeline;
import tale.vulkan.acceleration_structure;

namespace tale::vulkan {

struct Per_frame {
    Storage_texture render_texture;
    Tlas tlas;
};

export class Renderer {
public:
    Renderer(Context& context, Scene& scene, size_t size_command_buffers);
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;
    ~Renderer();

    void reset_swapchain(Context& context);
    void create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, size_t command_pool_size);
    void create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size);
    void trace(vk::CommandBuffer command_buffer, vk::Fence fence, size_t command_pool_id, const Scene& scene, vk::Extent2D extent);

private:
    vk::Device device;
    Monitor_swapchain swapchain;
    Raytracing_pipeline pipeline;
    std::vector<Per_frame> per_frame;
    Blas blas;
    size_t size_command_buffers;

    std::vector<vk::DescriptorSet> descriptor_sets;
};
}

module :private;

namespace tale::vulkan {
Renderer::Renderer(Context& context, Scene& scene, size_t size_command):
    device(context.device),
    swapchain(context, size_command),
    pipeline(context, scene),
    blas(context),
    size_command_buffers(size_command) {}

Renderer::~Renderer() { device.waitIdle(); }

void Renderer::trace(vk::CommandBuffer command_buffer, vk::Fence fence, size_t command_pool_id, const Scene& scene, vk::Extent2D extent) {
    command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    Per_frame& frame_data = per_frame[command_pool_id];
    frame_data.tlas.update(command_buffer, false, scene);
    std::array barriers{
        vk::BufferMemoryBarrier2KHR{
            .srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
            .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
            .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR,
            .buffer = frame_data.tlas.buffer.buffer, 
            .size = VK_WHOLE_SIZE
        }
    };
    command_buffer.pipelineBarrier2(
        vk::DependencyInfoKHR{.bufferMemoryBarrierCount = static_cast<uint32_t>(barriers.size()), .pBufferMemoryBarriers = barriers.data()}
    );

    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipeline.pipeline_layout, 0, descriptor_sets[command_pool_id], {});

    command_buffer.pushConstants(
        pipeline.pipeline_layout, vk::ShaderStageFlagBits::eRaygenKHR /* | vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eAnyHitKHR |
                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR*/
        ,
        0, sizeof(Camera), &scene.camera
    );

    command_buffer.traceRaysKHR(
        &pipeline.raygen_address_region, &pipeline.miss_address_region, &pipeline.hit_address_region, &pipeline.callable_address_region, extent.width,
        extent.height, 1u
    );

    swapchain.copy_image(command_buffer, frame_data.render_texture.image.image, command_pool_id, extent);
    command_buffer.end();
    swapchain.present(command_buffer, fence, command_pool_id);
}

void Renderer::reset_swapchain(Context& context) {
    device.waitIdle();
    // We want to call the destructor before the constructor
    swapchain.~Monitor_swapchain();
    [[maybe_unused]] Monitor_swapchain* s = new (&swapchain) Monitor_swapchain(context, size_command_buffers);
}

void Renderer::create_per_frame_data(Context& context, Scene& scene, vk::Extent2D extent, size_t command_pool_size) {
    per_frame.reserve(command_pool_size);
    One_time_command_buffer command_buffer(device, context.command_pool, context.queue);
    for (size_t i = 0u; i < command_pool_size; i++) {
        per_frame.push_back(Per_frame{.render_texture = Storage_texture(context, extent, command_buffer.command_buffer), .tlas = {context, blas, scene}});
    }
}

void Renderer::create_descriptor_sets(vk::DescriptorPool descriptor_pool, size_t command_pool_size) {
    const std::vector<vk::DescriptorSetLayout> layouts(command_pool_size, pipeline.descriptor_set_layout);
    descriptor_sets = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
        .descriptorPool = descriptor_pool, .descriptorSetCount = static_cast<uint32_t>(layouts.size()), .pSetLayouts = layouts.data()
    });

    for (size_t i = 0; i < command_pool_size; i++) {
        const vk::WriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{
            .accelerationStructureCount = 1u, .pAccelerationStructures = &(per_frame[i].tlas.acceleration_structure)
        };
        const vk::DescriptorImageInfo image_info{.imageView = per_frame[i].render_texture.image_view, .imageLayout = vk::ImageLayout::eGeneral};

        device.updateDescriptorSets(
            std::array{
                vk::WriteDescriptorSet{
                    .pNext = &descriptor_acceleration_structure_info,
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eAccelerationStructureKHR
                },
                vk::WriteDescriptorSet{
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eStorageImage,
                    .pImageInfo = &image_info
                }
            },
            {}
        );
    }
}
}