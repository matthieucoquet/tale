module;
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <vma_includes.hpp>
export module tale.vulkan.acceleration_structure;
import std;
import vulkan_hpp;
import tale.scene;
import tale.vulkan.context;
import tale.vulkan.buffer;
import tale.vulkan.command_buffer;

namespace tale::vulkan {
class Acceleration_structure {
public:
    vk::AccelerationStructureKHR acceleration_structure;
    Vma_buffer buffer{};

    Acceleration_structure(Context& context);
    Acceleration_structure(const Acceleration_structure& other) = delete;
    Acceleration_structure(Acceleration_structure&& other) noexcept;
    Acceleration_structure& operator=(const Acceleration_structure& other) = default;
    Acceleration_structure& operator=(Acceleration_structure&& other) noexcept;
    ~Acceleration_structure();

protected:
    vk::Device device;
    Vma_buffer scratch_buffer{};
    vk::DeviceAddress scratch_address;

    void create_buffers(Context& context, const vk::AccelerationStructureBuildSizesInfoKHR& build_size);
};

export class Blas : public Acceleration_structure {
public:
    vk::DeviceAddress address;

    Blas(Context& context);
    Blas(const Blas& other) = delete;
    Blas(Blas&& other) = delete;
    Blas& operator=(const Blas& other) = delete;
    Blas& operator=(Blas&& other) = delete;
    ~Blas() = default;

private:
    Vma_buffer aabb_buffer{};
};

export class Tlas : public Acceleration_structure {
public:
    Tlas(Context& context, const Blas& blas, Scene& scene);
    Tlas(const Tlas& other) = delete;
    Tlas(Tlas&& other) = default;
    Tlas& operator=(const Tlas& other) = delete;
    Tlas& operator=(Tlas&& other) = default;
    ~Tlas() = default;

    void update(vk::CommandBuffer command_buffer, bool first_build, const Scene& scene);

private:
    Vma_buffer instance_buffer{};
    vk::DeviceAddress blas_address;
    vk::AccelerationStructureGeometryKHR acceleration_structure_geometry;
};

}

module :private;

namespace tale::vulkan {

Acceleration_structure::Acceleration_structure(Context& context):
    device(context.device) {}

Acceleration_structure::Acceleration_structure(Acceleration_structure&& other) noexcept:
    acceleration_structure(other.acceleration_structure),
    device(other.device),
    buffer(std::move(other.buffer)),
    scratch_buffer(std::move(other.scratch_buffer)),
    scratch_address(other.scratch_address) {
    other.acceleration_structure = nullptr;
    other.device = nullptr;
}

Acceleration_structure& Acceleration_structure::operator=(Acceleration_structure&& other) noexcept {
    std::swap(acceleration_structure, other.acceleration_structure);
    std::swap(device, other.device);
    std::swap(buffer, other.buffer);
    std::swap(scratch_buffer, other.scratch_buffer);
    std::swap(scratch_address, other.scratch_address);
    return *this;
}

Acceleration_structure::~Acceleration_structure() {
    if (device) {
        device.destroyAccelerationStructureKHR(acceleration_structure);
        device = nullptr;
    }
}

void Acceleration_structure::create_buffers(Context& context, const vk::AccelerationStructureBuildSizesInfoKHR& build_size) {
    buffer = Vma_buffer(
        device, context.allocator,
        vk::BufferCreateInfo{
            .size = build_size.accelerationStructureSize,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive
        },
        VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE}
    );
    scratch_buffer = Vma_buffer(
        device, context.allocator,
        vk::BufferCreateInfo{
            .size = std::max(build_size.buildScratchSize, build_size.updateScratchSize),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive
        },
        VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE}
    );
    scratch_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = scratch_buffer.buffer});
}

Blas::Blas(Context& context):
    Acceleration_structure(context) {
    vk::AabbPositionsKHR aabb{-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

    aabb_buffer = Vma_buffer(
        device, context.allocator,
        vk::BufferCreateInfo{
            .size = sizeof(vk::AabbPositionsKHR) * 1,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress
        },
        VmaAllocationCreateInfo{.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO}
    );
    aabb_buffer.map();
    aabb_buffer.copy(reinterpret_cast<void*>(&aabb), 1 * sizeof(vk::AabbPositionsKHR));
    aabb_buffer.unmap();

    const vk::DeviceAddress aabb_buffer_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = aabb_buffer.buffer});

    vk::AccelerationStructureGeometryDataKHR geometry_data{};
    geometry_data.setAabbs(
        vk::AccelerationStructureGeometryAabbsDataKHR{.data = vk::DeviceOrHostAddressConstKHR(aabb_buffer_address), .stride = sizeof(vk::AabbPositionsKHR)}
    );

    const vk::AccelerationStructureGeometryKHR acceleration_structure_geometry{
        .geometryType = vk::GeometryTypeKHR::eAabbs, .geometry = geometry_data, .flags = vk::GeometryFlagBitsKHR::eOpaque
    };

    vk::AccelerationStructureBuildGeometryInfoKHR geometry_info{
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1u,
        .pGeometries = &acceleration_structure_geometry
    };

    const vk::AccelerationStructureBuildSizesInfoKHR build_size =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometry_info, 1u);
    create_buffers(context, build_size);

    acceleration_structure = device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .createFlags = {},
        .buffer = buffer.buffer,
        .offset = 0u,
        .size = build_size.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eBottomLevel
    });
    address = device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR{.accelerationStructure = acceleration_structure});

    geometry_info.dstAccelerationStructure = acceleration_structure;
    geometry_info.scratchData = vk::DeviceOrHostAddressKHR(scratch_address);

    const vk::AccelerationStructureBuildRangeInfoKHR build_range{.primitiveCount = 1u, .primitiveOffset = 0u, .firstVertex = 0u, .transformOffset = 0u};
    {
        One_time_command_buffer command_buffer(context.device, context.command_pool, context.queue);
        command_buffer.command_buffer.buildAccelerationStructuresKHR(geometry_info, &build_range);
    }
}

Tlas::Tlas(Context& context, const Blas& blas, Scene& scene):
    Acceleration_structure(context),
    blas_address(blas.address) {

    instance_buffer = Vma_buffer(
        context.device, context.allocator,
        vk::BufferCreateInfo{
            .size = sizeof(vk::AccelerationStructureInstanceKHR) * Scene::max_entities,
            .usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
        },
        VmaAllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO
        }
    );

    const vk::DeviceAddress instance_buffer_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = instance_buffer.buffer});

    vk::AccelerationStructureGeometryDataKHR geometry_data{};
    geometry_data.setInstances(
        vk::AccelerationStructureGeometryInstancesDataKHR{.arrayOfPointers = false, .data = vk::DeviceOrHostAddressConstKHR(instance_buffer_address)}
    );

    acceleration_structure_geometry = vk::AccelerationStructureGeometryKHR{.geometryType = vk::GeometryTypeKHR::eInstances, .geometry = geometry_data};

    const vk::AccelerationStructureBuildGeometryInfoKHR geometry_info{
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
        .geometryCount = 1u,
        .pGeometries = &acceleration_structure_geometry
    };
    const vk::AccelerationStructureBuildSizesInfoKHR build_size =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometry_info, Scene::max_entities);
    create_buffers(context, build_size);

    acceleration_structure = device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR{
        .createFlags = {},
        .buffer = buffer.buffer,
        .offset = 0u,
        .size = build_size.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eTopLevel
    });

    {
        One_time_command_buffer command_buffer(context.device, context.command_pool, context.queue);
        update(command_buffer.command_buffer, true, scene);
    }
}

void Tlas::update(vk::CommandBuffer command_buffer, bool first_build, const Scene& scene) {
    std::vector<vk::AccelerationStructureInstanceKHR> entities_instances{};
    for (const auto& entity : scene.entities) {
        glm::mat4 transform = glm::translate(entity.global_transform.position) * glm::toMat4(entity.global_transform.rotation) *
                              glm::scale(glm::vec3(entity.global_transform.scale));
        entities_instances.push_back(vk::AccelerationStructureInstanceKHR{
            .transform =
                {.matrix =
                     std::array<std::array<float, 4>, 3>{
                         std::array<float, 4>{transform[0][0], transform[1][0], transform[2][0], transform[3][0]},
                         std::array<float, 4>{transform[0][1], transform[1][1], transform[2][1], transform[3][1]},
                         std::array<float, 4>{transform[0][2], transform[1][2], transform[2][2], transform[3][2]}
                     }},
            .instanceCustomIndex = 0,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = static_cast<uint32_t>(entity.model_index),
            .accelerationStructureReference = blas_address
        });
    }
    instance_buffer.copy(reinterpret_cast<const void*>(entities_instances.data()), entities_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    instance_buffer.flush();

    const vk::AccelerationStructureBuildRangeInfoKHR build_range{
        .primitiveCount = static_cast<uint32_t>(scene.entities.size()), .primitiveOffset = 0u, .firstVertex = 0u, .transformOffset = 0u
    };
    command_buffer.buildAccelerationStructuresKHR(
        vk::AccelerationStructureBuildGeometryInfoKHR{
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
            .mode = first_build ? vk::BuildAccelerationStructureModeKHR::eBuild : vk::BuildAccelerationStructureModeKHR::eUpdate,
            .srcAccelerationStructure = first_build ? nullptr : acceleration_structure,
            .dstAccelerationStructure = acceleration_structure,
            .geometryCount = 1,
            .pGeometries = &acceleration_structure_geometry,
            .scratchData = scratch_address
        },
        &build_range
    );
}

}
