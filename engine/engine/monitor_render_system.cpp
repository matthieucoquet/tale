module;
export module tale.engine.monitor_render_system;
import std;
import tale.engine.system;
import tale.engine.shader_system;
import tale.scene;
import tale.window;
import tale.vulkan;

namespace tale::engine {
export class Monitor_render_system final : public System {
public:
    Monitor_render_system(Scene& scene, std::filesystem::path model_shader_path);
    Monitor_render_system(const Monitor_render_system& other) = delete;
    Monitor_render_system(Monitor_render_system&& other) = delete;
    Monitor_render_system& operator=(const Monitor_render_system& other) = delete;
    Monitor_render_system& operator=(Monitor_render_system&& other) = delete;
    ~Monitor_render_system() override final = default;

    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    Window window;
    vulkan::Context context;
    vulkan::Reusable_command_pools command_pools;
    engine::Shader_system shader_system;
    vulkan::Renderer renderer;
};
}

module :private;

namespace tale::engine {

static constexpr size_t size_command_buffers = 2u;
static constexpr vk::Extent2D init_windows_size{1920, 1080};

Monitor_render_system::Monitor_render_system(Scene& scene, std::filesystem::path model_shader_path):
    window(init_windows_size.width, init_windows_size.height),
    context(window),
    command_pools(context.device, context.queue_family, size_command_buffers),
    shader_system(context, scene, model_shader_path, false),
    renderer(context, scene, size_command_buffers) {
    renderer.create_per_frame_data(context, scene, init_windows_size, size_command_buffers);
    renderer.create_descriptor_sets(context.descriptor_pool, size_command_buffers);
}

bool Monitor_render_system::step(Scene& scene) {
    if (!window.step())
        return false;
    if (window.width != 0 && window.height != 0) {
        const size_t command_pool_id = command_pools.find_next();
        auto& command_buffer = command_pools.command_buffers[command_pool_id];
        auto fence = command_pools.fences[command_pool_id];
        renderer.start_frame(command_buffer, command_pool_id, scene);
        renderer.trace(command_buffer, command_pool_id, scene, init_windows_size);
        renderer.end_frame(command_buffer, fence, command_pool_id);
    }
    return true;
}

void Monitor_render_system::cleanup(Scene& scene) { shader_system.cleanup(scene); }

}