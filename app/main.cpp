#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
import std;
import tale.app;
import tale.scene;
import vulkan_hpp;
import tale.engine;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class Demo_app : public tale::App {
public:
    Demo_app() {
        scene.add_model("sphere");
        systems.push_back(std::make_unique<tale::engine::Physics_system>());
        systems.push_back(std::make_unique<tale::engine::Monitor_render_system>(scene, std::filesystem::path(SHADER_SOURCE)));
    }
};

int main() {
    spdlog::set_level(spdlog::level::info);

    Demo_app app{};
    app.run();

    return 0;
}
