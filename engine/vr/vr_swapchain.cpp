module;
// #include "vr_common.hpp"
// #include <spdlog/spdlog.h>
export module tale.vr.swapchain;
// import tale.vulkan.context;
// import tale.vr.instance;

// namespace tale::vr {
// export class Vr_swapchain {
// public:
//     static constexpr vk::Format required_format = vk::Format::eR16G16B16A16Sfloat;

//     Vr_swapchain(Instance& instance, xr::Session& session, vulkan::Context& context);
//     Vr_swapchain(const Vr_swapchain& other) = delete;
//     Vr_swapchain(Vr_swapchain&& other) = delete;
//     Vr_swapchain& operator=(const Vr_swapchain& other) = delete;
//     Vr_swapchain& operator=(Vr_swapchain&& other) = delete;
//     ~Vr_swapchain();

// private:
// };

// }

// module :private;

// namespace tale::vr {

// Vr_swapchain::Vr_swapchain(Instance& instance, xr::Session& session, vulkan::Context& context) {
//     const std::vector<int64_t> supported_formats = session.enumerateSwapchainFormatsToVector();

//     spdlog::debug("Supported Vulkan format in the XR runtime:");
//     for (auto format : supported_formats) {
//         spdlog::debug("\t{}", vk::to_string(static_cast<vk::Format>(format)));
//     }
//     if (std::ranges::none_of(supported_formats, supported_formats, [](const auto& format) { return static_cast<int64_t>(required_format) == format; })) {
//         throw std::runtime_error("Required format not supported by OpenXR runtime.");
//     }
// }

// }