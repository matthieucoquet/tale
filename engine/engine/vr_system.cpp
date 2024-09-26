module;
export module tale.engine.vr_system;
import std;
import tale.engine.system;
import tale.scene;
import tale.vr;

namespace tale::engine {
export class Vr_system final : public System {
public:
    Vr_system();
    Vr_system(const Vr_system& other) = delete;
    Vr_system(Vr_system&& other) = delete;
    Vr_system& operator=(const Vr_system& other) = delete;
    Vr_system& operator=(Vr_system&& other) = delete;
    ~Vr_system() override final = default;

    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    vr::Instance instance;
};
}

module :private;

namespace tale::engine {


Vr_system::Vr_system():
    instance() {

}

bool Vr_system::step(Scene& /*scene*/) {
    return true;
}

void Vr_system::cleanup(Scene& /*scene*/) {}

}