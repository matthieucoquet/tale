module;
export module tale.app;
import std;
import tale.scene;
import tale.engine;

namespace tale {
export class App {
public:
    App() = default;
    App(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;
    ~App();

    void run();

protected:
    Scene scene;
    std::vector<std::unique_ptr<engine::System>> systems;
};

}

module :private;

namespace tale {

App::~App() {
    for (auto& system : systems) {
        system->cleanup(scene);
    }
}

void App::run() {
    while (std::ranges::all_of(systems, [&scene = this->scene](auto& system) { return system->step(scene); })) {
    };
}
}