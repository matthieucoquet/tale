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
        const auto sphere_id = scene.add_model("sphere");
        const auto cube_id = scene.add_model("cube");

        scene.camera.pose.position = {-10.0f, 0.0f, 2.0f};

        scene.entities.push_back(tale::Entity{
            .global_transform = {.position = {0.0f, -0.5f, 1.5f}, .scale = 1.0f}, .model_index = sphere_id, .collision_shape = tale::Collision_shape::Sphere
        });
        scene.entities.push_back(tale::Entity{
            .global_transform = {.position = {0.0f, 0.1f, 3.5f}, .scale = 1.0f}, .model_index = cube_id, .collision_shape = tale::Collision_shape::Cube
        });

        systems.push_back(std::make_unique<tale::engine::Physics_system>(scene));
        systems.push_back(std::make_unique<tale::engine::Monitor_render_system>(scene, std::filesystem::path(SHADER_SOURCE)));
    }
};

int main() {
    spdlog::set_level(spdlog::level::info);

    Demo_app app{};
    app.run();

    return 0;
}
