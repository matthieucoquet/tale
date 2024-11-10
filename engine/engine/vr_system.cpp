module;
export module tale.engine.vr_system;
import std;
import tale.engine.system;
import tale.engine.shader_system;
import tale.scene;
import tale.vr;
import tale.window;
import tale.vulkan;

namespace tale::engine {
export class Vr_system final : public System {
public:
    Vr_system(Scene& scene, std::filesystem::path model_shader_path);
    Vr_system(const Vr_system& other) = delete;
    Vr_system(Vr_system&& other) = delete;
    Vr_system& operator=(const Vr_system& other) = delete;
    Vr_system& operator=(Vr_system&& other) = delete;
    ~Vr_system() override final = default;

    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    vr::Instance instance;
    Window window;
    vulkan::Context context;
    vr::Session session;
    vulkan::Reusable_command_pools command_pools;
    engine::Shader_system shader_system;
    vulkan::Renderer renderer;
};
}

module :private;

namespace tale::engine {

static constexpr size_t size_command_buffers = 3u;
static constexpr int init_windows_height{1080};

Vr_system::Vr_system(Scene& scene, std::filesystem::path model_shader_path):
    instance(),
    window(static_cast<int>(init_windows_height * instance.view_ratio), init_windows_height),
    context(window, &instance),
    session(instance),
    command_pools(context.device, context.queue_family, size_command_buffers),
    shader_system(context, scene, model_shader_path, true),
    renderer(context, scene, size_command_buffers) {

    const auto extent = session.swapchain.vk_view_extent();
    renderer.create_per_frame_data(context, scene, extent, size_command_buffers);
    renderer.create_descriptor_sets(context.descriptor_pool, size_command_buffers);
}

bool Vr_system::step(Scene& scene) {
    if (!window.step())
        return false;
    session.poll_events(instance.instance);
    if (session.start_frame(scene)) {
        const size_t command_pool_id = command_pools.find_next();
        auto& command_buffer = command_pools.command_buffers[command_pool_id];
        auto fence = command_pools.fences[command_pool_id];

        renderer.start_frame(command_buffer, command_pool_id, scene);
        auto source_image = renderer.trace(command_buffer, command_pool_id, scene, session.swapchain.vk_view_extent());
        session.copy_image(command_buffer, source_image);

        renderer.end_frame(command_buffer, fence, command_pool_id);

        session.end_frame();
    }
    return session.application_running;
}

void Vr_system::cleanup(Scene& scene) { shader_system.cleanup(scene); }
}