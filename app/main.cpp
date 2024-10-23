#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
import std;
import tale.app;
import tale.scene;
import vulkan_hpp;
import tale.engine;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void spawn_layer(std::vector<tale::Entity>& entities, int side, float height, float scale, size_t model_index) {
    for (int x = 0; x < side; ++x) {
        float x_pos = static_cast<float>(x) - static_cast<float>(side) / 2.0f;
        for (int y = 0; y < side; ++y) {
            float y_pos = static_cast<float>(y) - static_cast<float>(side) / 2.0f;
            entities.push_back(
                tale::Entity{.global_transform = {.position = {1.5f * scale * x_pos, 1.5f * scale * y_pos, height}, .scale = scale}, .model_index = model_index}
            );
        }
    }
}

class Demo_app : public tale::App {
public:
    Demo_app() {
        const auto inflate = glm::vec3(1.1f); // Inflate the bounding box to handle soft shadows
        const auto sphere_id = scene.add_model("sphere", tale::Collision_shape::Sphere, {glm::vec3(-0.5) - inflate, glm::vec3(0.5) + inflate});
        const auto cube_id = scene.add_model("cube", tale::Collision_shape::Cube, {glm::vec3(-0.5) - inflate, glm::vec3(0.5) + inflate});
        const auto floor_id =
            scene.add_model("floor", tale::Collision_shape::Plane, {glm::vec3(-50.0, -50.0, -1.0) - inflate, glm::vec3(50.0, 50.0, 0.0) + inflate});

        scene.camera.pose.position = {-20.0f, 0.0f, 3.0f};

        scene.entities.push_back(tale::Entity{.global_transform = {.position = {0.0f, 0.0f, 0.0f}, .scale = 1.0f}, .model_index = floor_id});
        spawn_layer(scene.entities, 5, 1.5f, 1.0f, sphere_id);
        spawn_layer(scene.entities, 3, 4.0f, 1.5f, cube_id);
        spawn_layer(scene.entities, 4, 5.5f, 1.5f, sphere_id);
        spawn_layer(scene.entities, 5, 7.0f, 1.0f, cube_id);

        scene.materials.push_back(tale::Material{.color = {0.8f, 0.1f, 0.1f, 1.0f}, .ks = 0.15f, .shininess = 32.0, .f0 = 0.2f});
        scene.materials.push_back(tale::Material{.color = {0.15f, 0.1f, 0.8f, 1.0f}, .ks = 0.15f, .shininess = 32.0, .f0 = 0.2f});
        scene.materials.push_back(tale::Material{.color = {0.95f, 0.95f, 0.95f, 1.0f}, .ks = 0.15f, .shininess = 32.0, .f0 = 0.2f});
        scene.materials.push_back(tale::Material{.color = {0.05f, 0.05f, 0.05f, 1.0f}, .ks = 0.15f, .shininess = 32.0, .f0 = 0.2f});

        scene.lights.push_back(tale::Light{.position = {-10.0f, 5.0f, 4.0f}, .color = {0.4f, 0.4f, 0.4f}});
        scene.lights.push_back(tale::Light{.position = {-10.0f, -20.0f, 1.0f}, .color = {0.1f, 0.1f, 0.1f}});

        systems.push_back(std::make_unique<tale::engine::Monitor_render_system>(scene, std::filesystem::path(SHADER_SOURCE)));
        // systems.push_back(std::make_unique<tale::engine::Vr_system>());

        systems.push_back(std::make_unique<tale::engine::Physics_system>(scene));
    }
};

int main() {
    spdlog::set_level(spdlog::level::debug);

    Demo_app app{};
    app.run();

    return 0;
}
