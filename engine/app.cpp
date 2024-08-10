module;
#include <compare>
// #include <imgui.h>
#include <string_view>
export module tale.app;
import tale.scene;
import tale.window;
import tale.vulkan;
import tale.engine;

namespace tale {
export class App {
public:
    App();
    App(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;
    ~App();

    void run();

private:
    // struct Imgui_context {
    //     Imgui_context() {
    //         IMGUI_CHECKVERSION();
    //         ImGui::CreateContext();
    //     }
    //     ~Imgui_context() { ImGui::DestroyContext(); }
    // };
    // Imgui_context imgui_context{}; // Need to be create before everything else

    Scene scene;
    Window window;
    vulkan::Context context;
    vulkan::Reusable_command_pools command_pools;
    engine::Shader_system shader_system;
    vulkan::Renderer renderer;
    // Input_system input_system;
    // Ui_system ui_system;
};

}

module :private;

static constexpr size_t size_command_buffers = 2u;
static constexpr vk::Extent2D init_windows_size{1920, 1080};

namespace tale {
App::App():
    scene(),
    window(init_windows_size.width, init_windows_size.height),
    context(window),
    command_pools(context.device, context.queue_family, size_command_buffers),
    shader_system(context, scene),
    renderer(context, scene, size_command_buffers)
//     input_system(window.window)
{
    renderer.create_per_frame_data(context, scene, init_windows_size, size_command_buffers);
    renderer.create_descriptor_sets(context.descriptor_pool, size_command_buffers);

    //     scene.screen_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
}

App::~App() { shader_system.cleanup(scene); }

void App::run() {
    while (window.step()) {
        if (window.was_resized()) {
            //         renderer.reset_swapchain(context);
            //         scene.screen_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        }
        //     scene.step();
        //     input_system.step();
        //     ui_system.step();

        const size_t command_pool_id = command_pools.find_next();
        auto& command_buffer = command_pools.command_buffers[command_pool_id];
        //     auto fence = command_pools.fences[command_pool_id];
        renderer.trace(command_buffer, command_pool_id, scene, init_windows_size);
    }
}
}