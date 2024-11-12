module;
#include "vr_common.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
export module tale.vr.session;
import std;
import tale.vr.instance;
import tale.vr.swapchain;
import vulkan_hpp;
import tale.scene;

namespace tale::vr {
export class Session {
public:
    xr::Session session;
    Vr_swapchain swapchain;
    bool session_running = false;
    bool application_running = true;
    vk::Image swapchain_image;

    Session(Instance& instance);
    Session(const Session& other) = delete;
    Session(Session&& other) = delete;
    Session& operator=(const Session& other) = delete;
    Session& operator=(Session&& other) = delete;
    ~Session();

    void poll_events(xr::Instance instance);
    // void draw_frame(Scene& scene, std::vector<std::unique_ptr<System>>& systems);
    void handle_state_change(xr::EventDataSessionStateChanged& event_stage_changed);
    [[nodiscard]] bool start_frame(Scene& scene);
    void copy_image(vk::CommandBuffer command_buffer, vk::Image source_image);
    void end_frame();

private:
    xr::SessionState session_state;
    xr::Space stage_space;
    xr::FrameState frame_state;

    xr::CompositionLayerProjection composition_layer{};
    std::array<xr::CompositionLayerProjectionView, 2> composition_layer_views;
};
}

module :private;

namespace tale::vr {

Session::Session(Instance& instance):
    session(instance.instance.createSession(xr::SessionCreateInfo(xr::SessionCreateFlagBits::None, instance.system_id, xr::get(instance.graphic_binding)))),
    swapchain(instance, session) {

    stage_space = session.createReferenceSpace(
        xr::ReferenceSpaceCreateInfo(xr::ReferenceSpaceType::Stage, xr::Posef(xr::Quaternionf(0.0f, 0.0f, 0.0f, 1.0f), xr::Vector3f(0.0f, 0.0f, 0.0f)))
    );

    for (size_t eye_id = 0u; eye_id < 2u; eye_id++) {
        const xr::Offset2Di offset(eye_id == 0 ? 0 : swapchain.view_extent.width, 0);
        composition_layer_views[eye_id] = xr::CompositionLayerProjectionView(
            xr::Posef(), xr::Fovf(), xr::SwapchainSubImage(swapchain.color_swapchain, xr::Rect2Di(offset, xr::Extent2Di(swapchain.view_extent)), 0)
        );
    }
    composition_layer =
        xr::CompositionLayerProjection(xr::CompositionLayerFlagBits::CorrectChromaticAberration, stage_space, 2u, composition_layer_views.data());
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
            session_running = false;
            application_running = false;
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

    session_state = event_stage_changed.state;
    switch (event_stage_changed.state) {
    case xr::SessionState::Ready: {
        session.beginSession(xr::SessionBeginInfo(xr::ViewConfigurationType::PrimaryStereo));
        session_running = true;
        break;
    }
    case xr::SessionState::Stopping: {
        session.endSession();
        session_running = false;
        break;
    }
    case xr::SessionState::Exiting: {
        session_running = false;
        application_running = false;
        break;
    }
    case xr::SessionState::LossPending: {
        session_running = false;
        application_running = false;
        break;
    }
    default:
        break;
    }
}

void update_camera(Camera& camera, xr::Posef pose, xr::Fovf fov) {
    // clang-format off
    static const glm::mat4 to_open_xr_frame = glm::mat4(
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    // clang-format on
    static const glm::mat4 from_open_xr_frame = glm::inverse(to_open_xr_frame);

    glm::quat xr_orientation = glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
    glm::vec4 xr_position = glm::vec4(pose.position.x, pose.position.y, pose.position.z, 1.0f);

    glm::mat4 rotation = from_open_xr_frame * glm::mat4_cast(xr_orientation) * to_open_xr_frame;

    camera.pose.rotation = glm::quat_cast(rotation);
    camera.pose.position = glm::vec3(from_open_xr_frame * xr_position);
    camera.fov.left = fov.angleLeft;
    camera.fov.right = fov.angleRight;
    camera.fov.up = fov.angleUp;
    camera.fov.down = fov.angleDown;
}

bool Session::start_frame(Scene& scene) {
    if (!session_running)
        return false;

    frame_state = session.waitFrame(xr::FrameWaitInfo());
    session.beginFrame(xr::FrameBeginInfo());
    const bool is_active =
        session_state == xr::SessionState::Synchronized || session_state == xr::SessionState::Visible || session_state == xr::SessionState::Focused;
    if (is_active && frame_state.shouldRender) {

        xr::ViewState view_state{};
        const xr::ViewLocateInfo view_locate_info(xr::ViewConfigurationType::PrimaryStereo, frame_state.predictedDisplayTime, stage_space);
        auto views = session.locateViewsToVector(view_locate_info, &(view_state.operator XrViewState&()));

        for (size_t eye_id = 0u; eye_id < 2u; eye_id++) {
            update_camera(scene.cameras[eye_id], views[eye_id].pose, views[eye_id].fov);
            scene.cameras[eye_id].pose.position += scene.center_play_area;

            composition_layer_views[eye_id].pose = views[eye_id].pose;
            composition_layer_views[eye_id].fov = views[eye_id].fov;
        }

        uint32_t swapchain_index = swapchain.color_swapchain.acquireSwapchainImage(xr::SwapchainImageAcquireInfo());
        swapchain.color_swapchain.waitSwapchainImage(xr::SwapchainImageWaitInfo(xr::Duration::infinite()));
        swapchain_image = swapchain.color_images[swapchain_index];
        return true;
    }
    session.endFrame(xr::FrameEndInfo(frame_state.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque, 0u, nullptr));
    return false;
}

void Session::copy_image(vk::CommandBuffer command_buffer, vk::Image source_image) {
    {
        vk::ImageMemoryBarrier2 memory_barrier{
            .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
            .srcAccessMask = {},
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .image = swapchain_image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
        };
        command_buffer.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &memory_barrier});
    }

    command_buffer.blitImage(
        source_image, vk::ImageLayout::eTransferSrcOptimal, swapchain_image, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageBlit{
            .srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0u, .baseArrayLayer = 0u, .layerCount = 1u},
            .srcOffsets =
                std::array{
                    vk::Offset3D{0, 0, 0},
                    vk::Offset3D{static_cast<int32_t>(swapchain.view_extent.width * 2), static_cast<int32_t>(swapchain.view_extent.height), 1}
                },
            .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0u, .baseArrayLayer = 0u, .layerCount = 1u},
            .dstOffsets =
                std::array{
                    vk::Offset3D{0, 0, 0},
                    vk::Offset3D{static_cast<int32_t>(swapchain.view_extent.width * 2), static_cast<int32_t>(swapchain.view_extent.height), 1}
                }
        },
        vk::Filter::eLinear
    );

    {
        vk::ImageMemoryBarrier2 memory_barrier{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .image = swapchain_image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1u, .baseArrayLayer = 0, .layerCount = 1}
        };
        command_buffer.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &memory_barrier});
    }
}

void Session::end_frame() {
    swapchain.color_swapchain.releaseSwapchainImage(xr::SwapchainImageReleaseInfo());
    std::vector<xr::CompositionLayerBaseHeader*> layers_pointers;
    layers_pointers.push_back(reinterpret_cast<xr::CompositionLayerBaseHeader*>(&composition_layer));
    session.endFrame(xr::FrameEndInfo(
        frame_state.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque, static_cast<uint32_t>(layers_pointers.size()), layers_pointers.data()
    ));
}
}