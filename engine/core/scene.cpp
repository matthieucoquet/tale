module;
#include <compare>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
export module tale.scene;
import std;
import vulkan_hpp;

namespace tale {

struct Pose {
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
};

struct Fov {
    float left = -0.5f;
    float right = 0.5f;
    float up = 0.28f;
    float down = -0.28f;
};

export struct Camera {
    Pose pose;
    Fov fov;
};

struct Shader {
    vk::ShaderModule module;
};

struct Model_shaders {
    std::string name;
    Shader primary_intersection;
    Shader primary_closest_hit;
};

struct Shaders {
    Shader raygen;
    Shader miss;
    std::vector<Model_shaders> models;
};

export struct Transform {
    glm::vec3 position{};
    glm::quat rotation{1.0, 0.0, 0.0, 0.0};
    float scale{1.0f};
};

export enum class Collision_shape { Sphere, Cube };

export struct Entity {
    Transform global_transform;
    size_t model_index;
    Collision_shape collision_shape;
};

export class Scene {
public:
    static constexpr unsigned int max_entities = 10u;

    Camera camera;
    std::vector<Entity> entities;
    Shaders shaders;

    Scene() { entities.reserve(max_entities); }

    size_t add_model(std::string model_name) {
        shaders.models.push_back(Model_shaders{.name = std::move(model_name)});
        return shaders.models.size() - 1;
    }
};
}