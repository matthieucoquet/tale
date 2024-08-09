module;
#include <vma_includes.hpp>
export module tale.vulkan.acceleration_structure;
import std;
import vulkan_hpp;
import tale.vulkan.context;
import tale.vulkan.buffer;
import tale.vulkan.command_buffer;

namespace tale::vulkan {
class Acceleration_structure {
public:
    Acceleration_structure(Context& context);
    Acceleration_structure(const Acceleration_structure& other) = delete;
    Acceleration_structure(Acceleration_structure&& other) noexcept;
    Acceleration_structure& operator=(const Acceleration_structure& other) = default;
    Acceleration_structure& operator=(Acceleration_structure&& other) noexcept;
    ~Acceleration_structure();

protected:
    vk::AccelerationStructureKHR acceleration_structure;
    vk::Device device;
};

export class Blas : Acceleration_structure {
public:
    vk::DeviceAddress address;

    Blas(Context& context);
    Blas(const Blas& other) = delete;
    Blas(Blas&& other) = delete;
    Blas& operator=(const Blas& other) = delete;
    Blas& operator=(Blas&& other) = delete;
    ~Blas() = default;

private:
    Vma_buffer buffer{};
    Vma_buffer aabbs_buffer{};
    Vma_buffer scratch_buffer{};
};

export class Tlas : Acceleration_structure {
public:
    Tlas(Context& context, const Blas& blas);
    Tlas(const Tlas& other) = delete;
    Tlas(Tlas&& other) = default;
    Tlas& operator=(const Tlas& other) = delete;
    Tlas& operator=(Tlas&& other) = default;
    ~Tlas() = default;
};

}

module :private;

namespace tale::vulkan {

Acceleration_structure::Acceleration_structure(Context& context):
    device(context.device) {}

Acceleration_structure::Acceleration_structure(Acceleration_structure&& other) noexcept:
    acceleration_structure(other.acceleration_structure),
    device(other.device) {
    other.acceleration_structure = nullptr;
    other.device = nullptr;
}

Acceleration_structure& Acceleration_structure::operator=(Acceleration_structure&& other) noexcept {
    std::swap(acceleration_structure, other.acceleration_structure);
    std::swap(device, other.device);
    return *this;
}

Acceleration_structure::~Acceleration_structure() {
    if (device) {
        device.destroyAccelerationStructureKHR(acceleration_structure);
        device = nullptr;
    }
}

Blas::Blas(Context& context):
    Acceleration_structure(context) {
    vk::AabbPositionsKHR aabb{-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

    aabbs_buffer = Vma_buffer(
        device, context.allocator,
        vk::BufferCreateInfo{
            .size = sizeof(vk::AabbPositionsKHR) * 1,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress
        },
        VmaAllocationCreateInfo{.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO}
    );
    aabbs_buffer.map();
    aabbs_buffer.copy(reinterpret_cast<void*>(&aabb), 1 * sizeof(vk::AabbPositionsKHR));
    aabbs_buffer.unmap();

    const vk::DeviceAddress aabb_buffer_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = aabbs_buffer.buffer});

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
    const vk::DeviceAddress scratch_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = scratch_buffer.buffer});

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

Tlas::Tlas(Context& context, const Blas& blas):
    Acceleration_structure(context) {

    vk::AccelerationStructureInstanceKHR instance{
        .transform =
            {.matrix =
                 std::array<std::array<float, 4>, 3>{
                     std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f}, std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f}, std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}
                 }},
        .instanceCustomIndex = 0,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .accelerationStructureReference = blas.address
    };
}

}
