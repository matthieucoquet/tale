module;
#include "PxConfig.h"
#include "PxPhysicsAPI.h"
#include <spdlog/spdlog.h>
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
    ~Physics_system() override final;

    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    physx::PxDefaultAllocator allocator;
    physx::PxDefaultErrorCallback error_callback;
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
    physx::PxScene* scene = nullptr;
    physx::PxMaterial* material = nullptr;
    physx::PxPvd* pvd = nullptr;
    physx::PxRigidDynamic* sphere = nullptr;
};
}

module :private;

constexpr bool use_pvd = false;

namespace tale::engine {
Physics_system::Physics_system() {
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, error_callback);

    if constexpr (use_pvd) {
        pvd = PxCreatePvd(*foundation);
        physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
    }

    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), false, pvd);

    dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    physx::PxSceneDesc scene_descriptor(physics->getTolerancesScale());
    scene_descriptor.gravity = physx::PxVec3(0.0f, 0.0f, -9.81f);
    scene_descriptor.cpuDispatcher = dispatcher;
    scene_descriptor.filterShader = physx::PxDefaultSimulationFilterShader;
    scene = physics->createScene(scene_descriptor);

    material = physics->createMaterial(0.5f, 0.5f, 0.6f);

    physx::PxRigidStatic* ground_plane = PxCreatePlane(*physics, physx::PxPlane(0.0f, 0.0f, 1.0f, 0.0f), *material);
    scene->addActor(*ground_plane);

    physx::PxShape* shape = physics->createShape(physx::PxSphereGeometry(0.5f), *material);
    physx::PxTransform transform(physx::PxVec3(0.0f, 0.0f, 2.0f));
    sphere = physics->createRigidDynamic(transform);
    sphere->attachShape(*shape);
    physx::PxRigidBodyExt::updateMassAndInertia(*sphere, 10.0f);
    scene->addActor(*sphere);

    shape->release();
}

Physics_system::~Physics_system() {
    PX_RELEASE(scene);
    PX_RELEASE(dispatcher);
    PX_RELEASE(physics);
    if (pvd) {
        physx::PxPvdTransport* transport = pvd->getTransport();
        PX_RELEASE(pvd);
        PX_RELEASE(transport);
    }
    PX_RELEASE(foundation);
}

bool Physics_system::step(Scene& /*scene*/) {
    scene->simulate(1.0f / 60.0f);
    scene->fetchResults(true);

    const physx::PxTransform transform = sphere->getGlobalPose();

    spdlog::info("{} {} {}", transform.p.x, transform.p.y, transform.p.z);

    return true;
}

void Physics_system::cleanup(Scene& /*scene*/) {}
}