#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
import tale.app;
import vulkan_hpp;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main() {
    spdlog::set_level(spdlog::level::info);

    auto app = tale::App();
    app.run();

    return 0;
}
