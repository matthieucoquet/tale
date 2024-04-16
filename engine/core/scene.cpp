module;
#include <glm/glm.hpp>
export module tale.scene;

namespace tale {

struct Sphere {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
};

export class Scene {
public:
    Sphere test_sphere;

    Scene() {}
};
}