#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
import std;
import tale.app;
import tale.scene;
import vulkan_hpp;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main() {
    spdlog::set_level(spdlog::level::info);

    tale::Scene scene{};
    scene.add_model("sphere");

    auto app = tale::App(std::move(scene), std::filesystem::path(SHADER_SOURCE));
    app.run();

    return 0;
}
