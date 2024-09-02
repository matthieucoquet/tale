module;
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hpp_macros.hpp>
export module tale.engine.shader_system;
import std;
import vulkan_hpp;
import tale.scene;
import tale.engine.system;
import tale.vulkan.context;

namespace tale::engine {

struct Shader_file {
    std::string name;
    std::string data{};
};

export class Shader_system final : public System {
public:
    Shader_system(vulkan::Context& context, Scene& scene, const std::filesystem::path& models_shader_path);
    Shader_system(const Shader_system& other) = delete;
    Shader_system(Shader_system&& other) = delete;
    Shader_system& operator=(const Shader_system& other) = delete;
    Shader_system& operator=(Shader_system&& other) = delete;
    ~Shader_system() override final = default;
    bool step(Scene& scene) override final;

    void cleanup(Scene& scene) override final;

private:
    vk::Device device;
    shaderc::Compiler compiler;
    shaderc::CompileOptions compile_options;
    std::vector<Shader_file> engine_files;
    std::vector<Shader_file> models_files;

    vk::ShaderModule compile(std::string_view shader_name, shaderc_shader_kind shader_kind, std::string_view model_name = {}) const;
    void read_files(const std::filesystem::path& models_shader_path);
    std::string read_file(std::filesystem::path path) const;
};
}

module :private;

namespace tale::engine {

class Includer : public shaderc::CompileOptions::IncluderInterface {
public:
    Includer(const std::vector<Shader_file>& engine_files, const std::vector<Shader_file>& models_files, std::string_view model_name):
        engine_files(engine_files),
        models_files(models_files),
        model_filename(std::string(model_name)) {
        if (!model_filename.empty()) {
            model_filename += ".glsl";
        }
    }

    shaderc_include_result*
    GetInclude(const char* requested_source, shaderc_include_type /*type*/, const char* /*requesting_source*/, size_t /*include_depth*/) override final {
        if (!model_filename.empty() && strcmp(requested_source, "model_map_function") == 0) {
            requested_source = model_filename.c_str();
        }

        auto is_requested_shader = [requested_source](const Shader_file& shader_file) { return shader_file.name == requested_source; };
        auto file_it = std::ranges::find_if(models_files, is_requested_shader);
        if (file_it == models_files.end()) {
            file_it = std::ranges::find_if(engine_files, is_requested_shader);
            assert(file_it != engine_files.end());
        }
        const Shader_file& included_shader = *file_it;
        data_holder.content = included_shader.data.data();
        data_holder.content_length = included_shader.data.size();
        data_holder.source_name = included_shader.name.c_str();
        data_holder.source_name_length = included_shader.name.size();
        data_holder.user_data = nullptr;
        return &data_holder;
    }

    void ReleaseInclude(shaderc_include_result* /*data*/) override final {}

private:
    shaderc_include_result data_holder;
    const std::vector<Shader_file>& engine_files;
    const std::vector<Shader_file>& models_files;
    std::string model_filename;
};

Shader_system::Shader_system(vulkan::Context& context, Scene& scene, const std::filesystem::path& models_shader_path):
    device(context.device) {

    read_files(models_shader_path);

    compile_options.SetSourceLanguage(shaderc_source_language_glsl);
    compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
    compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    compile_options.SetTargetSpirv(shaderc_spirv_version_1_6);
    compile_options.SetWarningsAsErrors();

    scene.shaders.raygen.module = compile("raygen_monitor.rgen", shaderc_raygen_shader);
    scene.shaders.miss.module = compile("primary.rmiss", shaderc_miss_shader);
    for (auto& model : scene.shaders.models) {
        model.primary_intersection.module = compile("primary.rint", shaderc_intersection_shader, model.name);
        model.primary_closest_hit.module = compile("primary.rchit", shaderc_closesthit_shader, model.name);
    }
}

bool Shader_system::step(Scene& /*scene*/) { return true; }

void Shader_system::cleanup(Scene& scene) {
    device.destroyShaderModule(scene.shaders.raygen.module);
    device.destroyShaderModule(scene.shaders.miss.module);
    for (auto& model : scene.shaders.models) {
        device.destroyShaderModule(model.primary_intersection.module);
        device.destroyShaderModule(model.primary_closest_hit.module);
    }
}

vk::ShaderModule Shader_system::compile(std::string_view shader_name, shaderc_shader_kind shader_kind, std::string_view model_name) const {

    auto file_it = std::ranges::find_if(engine_files, [shader_name](const Shader_file& shader_file) { return shader_file.name == shader_name; });
    assert(file_it != engine_files.end());
    const auto& shader_code = file_it->data;

    auto model_compile_options = compile_options;
    model_compile_options.SetIncluder(std::make_unique<Includer>(engine_files, models_files, model_name));
    auto compile_result = compiler.CompileGlslToSpv(shader_code.data(), shader_code.size(), shader_kind, shader_name.data(), model_compile_options);
    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        spdlog::error("GLSL compilation error: {}", compile_result.GetErrorMessage());
        return vk::ShaderModule{};
    } else {
        size_t code_size = sizeof(shaderc::SpvCompilationResult::element_type) * std::distance(compile_result.begin(), compile_result.end());
        return device.createShaderModule(vk::ShaderModuleCreateInfo{.codeSize = code_size, .pCode = compile_result.begin()});
    }
}

void Shader_system::read_files(const std::filesystem::path& models_shader_path) {
    std::filesystem::path engine_shader_path{SHADER_SOURCE};

    for (const auto& directory_entry : std::filesystem::directory_iterator(engine_shader_path)) {
        const auto& path = directory_entry.path();
        engine_files.emplace_back(Shader_file{.name = path.filename().string(), .data = read_file(path)});
    }
    for (const auto& directory_entry : std::filesystem::directory_iterator(models_shader_path)) {
        const auto& path = directory_entry.path();
        models_files.emplace_back(Shader_file{.name = path.filename().string(), .data = read_file(path)});
    }
}

std::string Shader_system::read_file(std::filesystem::path path) const {
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
    std::string buffer("");
    buffer.resize(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}
}