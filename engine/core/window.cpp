module;
#include <spdlog/spdlog.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
export module tale.window;
import std;
import vulkan_hpp;

namespace tale {
export class Window {
public:
    GLFWwindow* window;
    int width = 0;
    int height = 0;

    Window(int width, int height);
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window& operator=(const Window& other) = delete;
    Window& operator=(Window&& other) = delete;
    ~Window();

    [[nodiscard]] bool step();
    [[nodiscard]] std::vector<const char*> required_extensions() const;
    [[nodiscard]] vk::SurfaceKHR create_surface(vk::Instance instance) const;

    [[nodiscard]] bool was_resized() {
        bool was_resized = resized;
        resized = false;
        return was_resized;
    }

    friend void resize_callback(GLFWwindow* glfw_window, int width, int height);

private:
    bool resized = false;
};
}

module :private;

namespace tale {

void resize_callback(GLFWwindow* glfw_window, int width, int height) {
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->width = width;
    window->height = height;
    window->resized = true;
}

Window::Window(int w, int h):
    width(w),
    height(h) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "tale", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, resize_callback);
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::step() {
    if (glfwWindowShouldClose(window))
        return false;
    glfwPollEvents();
    return true;
}

std::vector<const char*> Window::required_extensions() const {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    spdlog::debug("GLFW required extensions:");
    for (const auto& ext : extensions) {
        spdlog::debug("\t{}", ext);
    }
    return extensions;
}

vk::SurfaceKHR Window::create_surface(vk::Instance instance) const {
    VkSurfaceKHR c_surface;
    vk::Result result = static_cast<vk::Result>(glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &c_surface));
    vk::resultCheck(result, "glfwCreateWindowSurface");
    return vk::createResultValueType(result, c_surface);
}

}