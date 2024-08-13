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

struct Sphere {
    glm::vec3 position = {10.0f, 0.0f, 0.0f};
    float radius = 1.0f;
};

struct Shader {
    vk::ShaderModule module;
};

struct Shader_group {
    std::string name;
    Shader primary_intersection;
    Shader primary_closest_hit;
};

struct Shaders {
    Shader raygen;
    Shader miss;
    std::vector<Shader_group> groups;
};

export class Scene {
public:
    Camera camera;
    Sphere test_sphere;
    Shaders shaders;

    Scene() {}

    void add_model(std::string model_name) { 
        shaders.groups.push_back(Shader_group{.name = std::move(model_name)});
    }
};
}