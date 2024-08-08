module;
#include <compare>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
export module tale.scene;
import vulkan_hpp;

namespace tale {

struct Pose {
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
};

struct Fov {
    float left = -0.5f;
    float right = 0.5f;
    float up = 0.5f;
    float down = -0.5f;
};

export struct Camera {
    Pose pose;
    Fov fov;
};

struct Sphere {
    glm::vec3 position = {0.0f, 10.0f, 0.0f};
    float radius = 1.0f;
};

struct Shader {
    vk::ShaderModule module;
};

struct Shaders {
    Shader raygen;
    Shader miss;
    Shader intersection;
    Shader closest_hit;
};

export class Scene {
public:
    Camera camera;
    Sphere test_sphere;
    Shaders shaders;

    Scene() {}
};
}