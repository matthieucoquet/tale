module;
export module tale.engine.system;
import tale.scene;

namespace tale::engine {

export class System {
public:
    virtual ~System() = default;
    virtual bool step(Scene& scene) = 0;
    virtual void cleanup(Scene& /*scene*/) {};
};

}