module;
#include "vr_common.hpp"
#include <spdlog/spdlog.h>
export module tale.vr.session;
import tale.vr.instance;
// import tale.vr.vr_swapchain;

namespace tale::vr {
export class Session {
public:
    Session(Instance& instance);
    Session(const Session& other) = delete;
    Session(Session&& other) = delete;
    Session& operator=(const Session& other) = delete;
    Session& operator=(Session&& other) = delete;
    ~Session();

    void poll_events(xr::Instance instance);
    // void draw_frame(Scene& scene, std::vector<std::unique_ptr<System>>& systems);
    void handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed);

private:
    xr::Session session;
    xr::SessionState session_state;
    // Vr_swapchain swapchain;
};
}

module :private;

namespace tale::vr {

Session::Session(Instance& instance) {
    xr::SessionCreateInfo session_create_info(xr::SessionCreateFlagBits::None, instance.system_id, xr::get(instance.graphic_binding));
    session = instance.instance.createSession(session_create_info);
}

Session::~Session() {
    if (session) {
        session.destroy();
    }
}

void Session::poll_events(xr::Instance instance) {
    while (true) {
        xr::EventDataBuffer data_buffer;
        if (instance.pollEvent(data_buffer) == xr::Result::EventUnavailable) {
            return;
        }

        switch (data_buffer.type) {
        case xr::StructureType::EventDataInstanceLossPending: {
            spdlog::warn("[Event] Instance loss pending");
            // TODO shut down instance and session;
            break;
        }
        case xr::StructureType::EventDataEventsLost: {
            spdlog::debug("[Event] Lost events");
            break;
        }
        case xr::StructureType::EventDataInteractionProfileChanged: {
            spdlog::debug("[Event] Interaction profile changed");
            break;
        }
        case xr::StructureType::EventDataReferenceSpaceChangePending: {
            spdlog::debug("[Event] Reference space changed");
            break;
        }
        case xr::StructureType::EventDataSessionStateChanged: {
            handle_state_change(reinterpret_cast<xr::EventDataSessionStateChanged&>(data_buffer));
            break;
        }
        }
    }
}

void Session::handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed) {
    spdlog::debug("[Event] State changed to {}", xr::to_string_literal(event_stage_changed.state));

    // m_session_state = event_stage_changed.state;
    switch (event_stage_changed.state) {
    case xr::SessionState::Ready: {
        session.beginSession(xr::SessionBeginInfo(xr::ViewConfigurationType::PrimaryStereo));
        break;
    }
    case xr::SessionState::Stopping: {
        session.endSession();
        break;
    }
    case xr::SessionState::Exiting: {
        // TODO shut down application
        break;
    }
    default:
        break;
    }
}

}