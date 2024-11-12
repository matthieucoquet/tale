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

struct Scene_shaders {
    Shader raygen;
    Shader primary_miss;
    Shader shadow_ao_miss;
    Shader shadow_ao_intersection;
};

struct Model_shaders {
    Shader primary_intersection;
    Shader primary_closest_hit;
    Shader shadow_any_hit;
    Shader ambient_occlusion_any_hit;
};

export struct Material {
    glm::vec4 color;
    float ks;
    float shininess;
    float f0;
};

export struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

export struct Transform {
    glm::vec3 position{};
    glm::quat rotation{1.0, 0.0, 0.0, 0.0};
    float scale{1.0f};
};

export enum class Collision_shape { Sphere, Cube, Plane };

export struct Model {
    std::string name;
    Model_shaders shaders;
    Collision_shape collision_shape;
    std::array<glm::vec3, 2> bounding_box;
};

export struct Entity {
    Transform global_transform;
    size_t model_index;
};

export class Scene {
public:
    static constexpr unsigned int max_entities = 100u;

    Scene_shaders shaders;
    std::vector<Model> models;

    glm::vec3 center_play_area;
    std::array<Camera, 2> cameras;
    std::vector<Entity> entities;

    std::vector<Material> materials;
    std::vector<Light> lights;

    Scene() { entities.reserve(max_entities); }

    size_t add_model(std::string model_name, Collision_shape collision_shape, std::array<glm::vec3, 2> bounding_box) {
        models.push_back(Model{.name = std::move(model_name), .collision_shape = collision_shape, .bounding_box = bounding_box});
        return models.size() - 1;
    }
};
}