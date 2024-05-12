module;
#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
export module tale.engine.shader_system;
import vulkan_hpp;
import tale.scene;
import tale.engine.system;
import tale.vulkan.context;

namespace tale::engine {
export class Shader_system final : public System {
public:
    Shader_system(vulkan::Context& context, Scene& scene);
    Shader_system(const Shader_system& other) = delete;
    Shader_system(Shader_system&& other) = delete;
    Shader_system& operator=(const Shader_system& other) = delete;
    Shader_system& operator=(Shader_system&& other) = delete;
    ~Shader_system() override final = default;
    void step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    vk::Device device;
    shaderc::Compiler compiler;
    shaderc::CompileOptions compile_options;
    vk::ShaderModule compile(std::string_view shader_name, shaderc_shader_kind shader_kind) const;
    std::vector<char> read_file(std::filesystem::path path) const;
};
}

module :private;

namespace tale::engine {

Shader_system::Shader_system(vulkan::Context& context, Scene& scene):
    device(context.device) {

    compile_options.SetSourceLanguage(shaderc_source_language_glsl);
    compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
    compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    compile_options.SetTargetSpirv(shaderc_spirv_version_1_6);
    compile_options.SetWarningsAsErrors();

    scene.shaders.raygen.module = compile("raygen_monitor.rgen", shaderc_raygen_shader);
    scene.shaders.miss.module = compile("primary.rmiss", shaderc_miss_shader);
    scene.shaders.intersection.module = compile("primary.rint", shaderc_intersection_shader);
    scene.shaders.closest_hit.module = compile("primary.rchit", shaderc_closesthit_shader);
}

void Shader_system::step(Scene& /*scene*/) {}

void Shader_system::cleanup(Scene& scene) {
    device.destroyShaderModule(scene.shaders.raygen.module);
    device.destroyShaderModule(scene.shaders.miss.module);
    device.destroyShaderModule(scene.shaders.intersection.module);
    device.destroyShaderModule(scene.shaders.closest_hit.module);
}

vk::ShaderModule Shader_system::compile(std::string_view shader_name, shaderc_shader_kind shader_kind) const {
    std::filesystem::path shader_path{SHADER_SOURCE};
    const std::vector<char> shader_code = read_file(shader_path / shader_name);

    auto compile_result = compiler.CompileGlslToSpv(shader_code.data(), shader_code.size(), shader_kind, shader_name.data(), compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        spdlog::error("GLSL compilation error: {}", compile_result.GetErrorMessage());
        return vk::ShaderModule{};
    } else {
        size_t code_size = sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end());
        return device.createShaderModule(vk::ShaderModuleCreateInfo{.codeSize = code_size, .pCode = compile_result.begin()});
    }
}

std::vector<char> Shader_system::read_file(std::filesystem::path path) const {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader path doesn't exist on filesystem.");
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Shader path is not a regular file.");
    }

    std::ifstream file(path.string(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}
}