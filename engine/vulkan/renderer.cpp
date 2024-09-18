
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
    Vma_buffer materials;
    Vma_buffer lights;
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
    std::vector<Blas> blas; // One per model
    size_t size_command_buffers;

    std::vector<vk::DescriptorSet> descriptor_sets;

    void update_per_frame_data(const Scene& scene, size_t command_pool_id);
};
}

module :private;

namespace tale::vulkan {
Renderer::Renderer(Context& context, Scene& scene, size_t size_command):
    device(context.device),
    swapchain(context, size_command),
    pipeline(context, scene),
    size_command_buffers(size_command) {
    blas.reserve(scene.models.size());
    for (const auto& model : scene.models) {
        blas.push_back(Blas(context, model));
    }
    }

Renderer::~Renderer() { device.waitIdle(); }

void Renderer::trace(vk::CommandBuffer command_buffer, vk::Fence fence, size_t command_pool_id, const Scene& scene, vk::Extent2D extent) {
    update_per_frame_data(scene, command_pool_id);

    command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    Per_frame& frame_data = per_frame[command_pool_id];
    frame_data.tlas.update(command_buffer, false, scene);
    std::array barriers{vk::BufferMemoryBarrier2KHR{
        .srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
        .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR,
        .buffer = frame_data.tlas.buffer.buffer,
        .size = VK_WHOLE_SIZE
    }};
    command_buffer.pipelineBarrier2(
        vk::DependencyInfoKHR{.bufferMemoryBarrierCount = static_cast<uint32_t>(barriers.size()), .pBufferMemoryBarriers = barriers.data()}
    );

    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipeline.pipeline_layout, 0, descriptor_sets[command_pool_id], {});

    command_buffer.pushConstants(
        pipeline.pipeline_layout,
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eClosestHitKHR |
        vk::ShaderStageFlagBits::eAnyHitKHR /*| vk::ShaderStageFlagBits::eMissKHR*/
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
        Vma_buffer material_buffer = Vma_buffer(
            context.device, context.allocator,
            vk::BufferCreateInfo{.size = sizeof(Material) * scene.materials.size(), .usage = vk::BufferUsageFlagBits::eStorageBuffer},
            VmaAllocationCreateInfo{
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO
            }
        );
        Vma_buffer lights_buffer = Vma_buffer(
            context.device, context.allocator,
            vk::BufferCreateInfo{.size = sizeof(Light) * scene.lights.size(), .usage = vk::BufferUsageFlagBits::eStorageBuffer},
            VmaAllocationCreateInfo{
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO
            }
        );
        per_frame.push_back(Per_frame{
            .render_texture = Storage_texture(context, extent, command_buffer.command_buffer),
            .tlas = {context, blas, scene},
            .materials = std::move(material_buffer),
            .lights = std::move(lights_buffer),
        });
    }
}

void Renderer::update_per_frame_data(const Scene& scene, size_t command_pool_id) {
    per_frame[command_pool_id].materials.copy(scene.materials.data(), sizeof(Material) * scene.materials.size());
    per_frame[command_pool_id].lights.copy(scene.lights.data(), sizeof(Light) * scene.lights.size());
    per_frame[command_pool_id].materials.flush();
    per_frame[command_pool_id].lights.flush();
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

        const vk::DescriptorBufferInfo material_info{.buffer = per_frame[i].materials.buffer, .offset = 0u, .range = VK_WHOLE_SIZE};
        const vk::DescriptorBufferInfo light_info{.buffer = per_frame[i].lights.buffer, .offset = 0u, .range = VK_WHOLE_SIZE};

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
                },
                vk::WriteDescriptorSet{
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eStorageBuffer,
                    .pBufferInfo = &material_info
                },
                vk::WriteDescriptorSet{
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 3,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eStorageBuffer,
                    .pBufferInfo = &light_info
                },
            },
            {}
        );
    }
}
}