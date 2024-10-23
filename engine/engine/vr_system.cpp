module;
export module tale.engine.vr_system;
import std;
import tale.engine.system;
import tale.scene;
import tale.vr;
import tale.window;
import tale.vulkan;

namespace tale::engine {
export class Vr_system final : public System {
public:
    Vr_system();
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
};
}

module :private;

namespace tale::engine {

// static constexpr size_t size_command_buffers = 2u;
static constexpr vk::Extent2D init_windows_size{1920, 1080}; // TODO fix ratio

Vr_system::Vr_system():
    instance(),
    window(init_windows_size.width, init_windows_size.height),
    context(window, &instance),
    session(instance) {}

bool Vr_system::step(Scene& /*scene*/) {
    session.poll_events(instance.instance);
    return true;
}

void Vr_system::cleanup(Scene& /*scene*/) {}

}