module;
#include <compare>
#include <glm/glm.hpp>
export module tale.scene;
import vulkan_hpp;

namespace tale {

struct Sphere {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
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
    Sphere test_sphere;
    Shaders shaders;

    Scene() {}
};
}