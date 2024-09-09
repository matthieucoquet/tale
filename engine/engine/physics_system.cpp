module;
#include "PxConfig.h"
#include "PxPhysicsAPI.h"
#include <spdlog/spdlog.h>
export module tale.engine.physics_system;
import tale.engine.system;
import tale.scene;

namespace tale::engine {
export class Physics_system final : public System {
public:
    Physics_system(Scene& scene);
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
    physx::PxScene* physics_scene = nullptr;
    physx::PxMaterial* material = nullptr;
    physx::PxPvd* pvd = nullptr;
};
}

module :private;

constexpr bool use_pvd = false;

namespace tale::engine {
Physics_system::Physics_system(Scene& scene) {
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
    physics_scene = physics->createScene(scene_descriptor);

    material = physics->createMaterial(0.5f, 0.5f, 0.6f);

    for (auto& entity : scene.entities) {
        physx::PxTransform transform(physx::PxVec3(entity.global_transform.position.x, entity.global_transform.position.y, entity.global_transform.position.z));
        if (entity.collision_shape == Collision_shape::Plane) {
            physx::PxRigidStatic* ground_plane = PxCreatePlane(*physics, physx::PxPlane(0.0f, 0.0f, 1.0f, 0.0f), *material);
            physics_scene->addActor(*ground_plane);
        } else {
            physx::PxShape* shape = nullptr; // TODO reuse shapes
            const auto scale = entity.global_transform.scale;
            if (entity.collision_shape == Collision_shape::Sphere) {
                shape = physics->createShape(physx::PxSphereGeometry(0.5f * scale), *material);
            } else if (entity.collision_shape == Collision_shape::Cube) {
                shape = physics->createShape(physx::PxBoxGeometry(0.5f * scale, 0.5f * scale, 0.5f * scale), *material);
            }
            physx::PxRigidDynamic* body = physics->createRigidDynamic(transform);
            body->attachShape(*shape);
            body->userData = static_cast<void*>(&entity);
            physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
            physics_scene->addActor(*body);
            shape->release();
        }
    }
}

Physics_system::~Physics_system() {
    PX_RELEASE(physics_scene);
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
    physics_scene->simulate(1.0f / 60.0f);
    physics_scene->fetchResults(true);

    const auto nb_actors = physics_scene->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC);
    if (nb_actors) {
        std::vector<physx::PxRigidActor*> actors(nb_actors);
        physics_scene->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC, reinterpret_cast<physx::PxActor**>(&actors[0]), nb_actors);

        for (const auto& actor : actors) {
            auto* entity = reinterpret_cast<Entity*>(actor->userData);
            const physx::PxTransform transform = actor->getGlobalPose();
            entity->global_transform.position = {transform.p.x, transform.p.y, transform.p.z};

            auto q = transform.q.getNormalized();
            entity->global_transform.rotation = {q.w, q.x, q.y, q.z};
        }
    }
    return true;
}

void Physics_system::cleanup(Scene& /*scene*/) {}
}