module;
 #include <compare>
// #include <imgui.h>
// #include <string_view>
export module tale.app;
// import cell.input_system;
// import cell.ui_system;
import tale.window;
import tale.vulkan;
// import cell.context;
// import cell.renderer;
// import cell.command_buffer;
// import cell.scene;

namespace tale {
export class App {
public:
    App();
    App(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

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

    // Scene scene;
    Window window;
    vulkan::Context context;
    // vulkan::Reusable_command_pools command_pools;
    // vulkan::Renderer renderer;
    // Input_system input_system;
    // Ui_system ui_system;
};

}

module :private;

// static constexpr size_t size_command_buffers = 2u;

namespace tale {
App::App():
    //     scene(),
    window(1000, 800),
    context(window)
//     command_pools(context.device, context.queue_family, size_command_buffers),
//     renderer(context, size_command_buffers),
//     input_system(window.window)
{
    //     scene.screen_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
}

void App::run() {
    while (window.step()) {
        if (window.was_resized()) {
            //         renderer.reset_swapchain(context);
            //         scene.screen_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        }
        //     scene.step();
        //     input_system.step();
        //     ui_system.step();

        //     size_t command_pool_id = command_pools.find_next();
        //     auto& command_buffer = command_pools.command_buffers[command_pool_id];
        //     auto fence = command_pools.fences[command_pool_id];
        //     renderer.draw(scene, command_buffer, fence, command_pool_id);
    }
}
}