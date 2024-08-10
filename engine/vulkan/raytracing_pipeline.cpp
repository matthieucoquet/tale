module;
#include <vulkan/vulkan_hpp_macros.hpp>
export module tale.vulkan.raytracing_pipeline;
import vulkan_hpp;
import std;
import tale.vulkan.context;
import tale.scene;
import tale.vulkan.command_buffer;
import tale.vulkan.buffer;

namespace tale::vulkan {
export class Raytracing_pipeline {
public:
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;

    vk::StridedDeviceAddressRegionKHR raygen_address_region{};
    vk::StridedDeviceAddressRegionKHR miss_address_region{};
    vk::StridedDeviceAddressRegionKHR hit_address_region{};
    vk::StridedDeviceAddressRegionKHR callable_address_region{};

    Raytracing_pipeline(Context& context, Scene& scene);
    Raytracing_pipeline(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline(Raytracing_pipeline&& other) = delete;
    Raytracing_pipeline& operator=(const Raytracing_pipeline& other) = delete;
    Raytracing_pipeline& operator=(Raytracing_pipeline&& other) = delete;
    ~Raytracing_pipeline();

private:
    vk::Device device;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties;
    Vma_buffer shader_binding_table{};

    uint32_t group_count;
    vk::DeviceSize offset_miss_group;
    vk::DeviceSize offset_hit_group;

    void create_pipeline(Scene& scene);
    void create_shader_binding_table(Context& context);
};
}

module :private;

namespace tale::vulkan {

Raytracing_pipeline::Raytracing_pipeline(Context& context, Scene& scene):
    device(context.device) {

    vk::PhysicalDeviceProperties2 properties{};
    properties.pNext = &raytracing_properties;
    context.physical_device.getProperties2(&properties);

    create_pipeline(scene);
    create_shader_binding_table(context);
}

Raytracing_pipeline::~Raytracing_pipeline() {
    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(pipeline_layout);
    device.destroyDescriptorSetLayout(descriptor_set_layout);
}

void Raytracing_pipeline::create_pipeline(Scene& scene) {
    const std::array bindings{
        // Acceleration structure
        vk::DescriptorSetLayoutBinding{
            .binding = 0u,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1u,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
        },
        // Output storage image
        vk::DescriptorSetLayoutBinding{
            .binding = 1u, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1u, .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
        }
    };

    descriptor_set_layout =
        device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()}
        );

    const vk::PushConstantRange push_constants{.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR, .offset = 0, .size = sizeof(Camera)};

    pipeline_layout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = 1u, .pSetLayouts = &descriptor_set_layout, .pushConstantRangeCount = 1u, .pPushConstantRanges = &push_constants
    });

    const std::vector shader_stages{
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eRaygenKHR, .module = scene.shaders.raygen.module, .pName = "main"},
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eMissKHR, .module = scene.shaders.miss.module, .pName = "main"},
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eIntersectionKHR, .module = scene.shaders.intersection.module, .pName = "main"},
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eClosestHitKHR, .module = scene.shaders.closest_hit.module, .pName = "main"},
    };
    const std::vector groups{
        // Raygen
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // raygen shader id
        },
        // Miss
        vk::RayTracingShaderGroupCreateInfoKHR{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // miss shader id
        },
        vk::RayTracingShaderGroupCreateInfoKHR{.type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup, .closestHitShader = 3, .intersectionShader = 2},
    };
    group_count = static_cast<uint32_t>(groups.size());

    pipeline = device
                   .createRayTracingPipelineKHR(
                       nullptr, nullptr,
                       vk::RayTracingPipelineCreateInfoKHR{
                           .flags = vk::PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR,
                           .stageCount = static_cast<uint32_t>(shader_stages.size()),
                           .pStages = shader_stages.data(),
                           .groupCount = static_cast<uint32_t>(groups.size()),
                           .pGroups = groups.data(),
                           .maxPipelineRayRecursionDepth = 1,
                           .layout = pipeline_layout
                       }
                   )
                   .value;
}

constexpr uint32_t align_up(uint32_t value, size_t alignment) noexcept { return uint32_t((value + (uint32_t(alignment) - 1)) & ~uint32_t(alignment - 1)); }

void Raytracing_pipeline::create_shader_binding_table(Context& context) {
    const uint32_t handle_size = raytracing_properties.shaderGroupHandleSize;
    const uint32_t handle_size_aligned = align_up(handle_size, raytracing_properties.shaderGroupHandleAlignment);

    const uint32_t total_handles_size = handle_size * group_count;
    const std::vector<uint8_t> handles_data = device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pipeline, 0u, group_count, total_handles_size);

    const uint32_t base_alignment = raytracing_properties.shaderGroupBaseAlignment;
    raygen_address_region.size = align_up(handle_size_aligned, base_alignment);
    raygen_address_region.stride = raygen_address_region.size;
    miss_address_region.size = align_up(1 * handle_size_aligned, base_alignment);
    miss_address_region.stride = handle_size_aligned;
    hit_address_region.size = align_up(1 * handle_size_aligned, base_alignment);
    hit_address_region.stride = handle_size_aligned;

    const vk::DeviceSize table_size = raygen_address_region.size + miss_address_region.size + hit_address_region.size + callable_address_region.size;
    std::vector<uint8_t> temp_table(table_size, 0);

    offset_miss_group = raygen_address_region.size;
    offset_hit_group = offset_miss_group + miss_address_region.size;

    // Copy raygen
    memcpy(temp_table.data(), handles_data.data(), handle_size);
    // Copy miss
    memcpy(temp_table.data() + offset_miss_group, handles_data.data() + 1 * handle_size, handle_size);
    // Copy hit
    memcpy(temp_table.data() + offset_hit_group, handles_data.data() + 2 * handle_size, handle_size);

    {
        One_time_command_buffer command_buffer(context.device, context.command_pool, context.queue);
        Buffer_from_staged buffer_and_staged(
            context.device, context.allocator, command_buffer.command_buffer,
            vk::BufferCreateInfo{
                .size = temp_table.size(), .usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
            },
            temp_table.data()
        );
        shader_binding_table = std::move(buffer_and_staged.result);
    }

    const vk::DeviceAddress table_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = shader_binding_table.buffer});
    raygen_address_region.deviceAddress = table_address;
    miss_address_region.deviceAddress = table_address + offset_miss_group;
    hit_address_region.deviceAddress = table_address + offset_hit_group;
}

}