
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
import tale.vulkan.texture;
import tale.vulkan.raytracing_pipeline;

namespace tale::vulkan {

struct Per_frame {
    Storage_texture render_texture;
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

private:
    vk::Device device;
    Monitor_swapchain swapchain;
    Raytracing_pipeline pipeline;
    std::vector<Per_frame> per_frame;
};
}

module :private;

namespace tale::vulkan {
Renderer::Renderer(Context& context, Scene& scene, size_t /*size_command_buffers*/):
    device(context.device),
    swapchain(context),
    pipeline(context, scene) {}

Renderer::~Renderer() { device.waitIdle(); }

void Renderer::reset_swapchain(Context& context) {
    device.waitIdle();
    // We want to call the destructor before the constructor
    swapchain.~Monitor_swapchain();
    [[maybe_unused]] Monitor_swapchain* s = new (&swapchain) Monitor_swapchain(context);
}

void Renderer::create_per_frame_data(Context& context, Scene& /*scene*/, vk::Extent2D extent, size_t command_pool_size) {
    per_frame.reserve(command_pool_size);
    One_time_command_buffer command_buffer(device, context.command_pool, context.queue);
    for (size_t i = 0u; i < command_pool_size; i++) {
        per_frame.push_back(Per_frame{.render_texture = Storage_texture(context, extent, command_buffer.command_buffer)});
    }
}
}