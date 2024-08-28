module;
#include "PxPhysicsAPI.h"
export module tale.engine.physics_system;
import tale.engine.system;

namespace tale::engine {
export class Physics_system final : public System {
public:
    Physics_system();
    Physics_system(const Physics_system& other) = delete;
    Physics_system(Physics_system&& other) = delete;
    Physics_system& operator=(const Physics_system& other) = delete;
    Physics_system& operator=(Physics_system&& other) = delete;
    ~Physics_system() override final = default;

    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;
};
}

module :private;

namespace tale::engine {
Physics_system::Physics_system() {
    physx::PxDefaultAllocator gAllocator;
    physx::PxDefaultErrorCallback gErrorCallback;
    physx::PxFoundation* gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    PX_RELEASE(gFoundation);
}

bool Physics_system::step(Scene& /*scene*/) { return true; }

void Physics_system::cleanup(Scene& /*scene*/) {}
}