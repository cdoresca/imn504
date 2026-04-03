#include "shader.hpp"

#include "shaderUtils.hpp"
#include <iostream>
#include <shaderc/shaderc.hpp> // sometimes <shaderc.hpp> works too depending on your installation
#include <spirv_reflect.h>
#include <stdexcept>


namespace Shader {

// define the global includer (declared extern in the header)
std::vector<std::filesystem::path> g_IncludePaths = {};

Shader::Shader()
    : m_hash(0),
      m_path(),
      m_source(),
      m_entryPoint("main"),
      m_kind(shaderc_glsl_infer_from_source),
      m_Binary(),
      m_stage(vk::ShaderStageFlagBits::eAll),
      descriptorSetInfos(), // default-initialized; setIndex = -1 because of struct default
      uboLayouts(),
      pushConstants(),
      hasChanged(false) {
#ifdef NDEBUG
    debug = false;
#else
    debug = true;
#endif
}

void Shader::load(const std::string &path, const std::string &entryPoint) {
    m_kind = ShaderUtils::getKindFromFileExt(path);
    m_source = ShaderUtils::readShaderSource(path);
    m_stage = ShaderUtils::getVkShaderStage(m_kind);
    m_entryPoint = entryPoint;
    m_path = path;

    std::string temp = preprocess(m_source);
    size_t hash = std::hash<std::string>{}(temp);
    if (m_hash != hash) {
        std::vector<uint32_t> binary;
        try {
            binary = compile(m_source, m_path, m_kind);
        } catch (std::runtime_error &e) {
            std::cout << "Failed shader compilation with: " << e.what() << std::endl;
            return;
        }
        try {
            reflect(binary);
        } catch (std::runtime_error &e) {
            std::cout << "Failed shader reflection with: " << e.what() << std::endl;
            return;
        }

        m_hash = hash;
        hasChanged = true;
        m_Binary = std::move(binary);
    } else {
        hasChanged = false;
    }
}

vk::ShaderStageFlagBits Shader::getStage() const { return m_stage; }
bool Shader::changed() const { return hasChanged; }
std::string Shader::getPath() const { return m_path; }
std::string Shader::getSource() const { return m_source; }
std::vector<uint32_t> Shader::getBinary() const { return m_Binary; }
std::array<DescriptorSetInfo, EE::Config::kMaxDescriptorSets> Shader::getDescriptorSetInfos() const { return descriptorSetInfos; }

vk::ShaderModuleCreateInfo Shader::getShaderCreateInfo() {
    return {{}, m_Binary.size() * sizeof(uint32_t), m_Binary.data()};
}

vk::PipelineShaderStageCreateInfo Shader::getPipelineShaderStageCreateInfo() {
    return vk::PipelineShaderStageCreateInfo({}, getStage(), nullptr, m_entryPoint.c_str());
}

std::vector<RHI::UBOLayout> Shader::getUBOs() { return uboLayouts; }
RHI::UBOLayout Shader::getPushConstants() { return pushConstants; }
void Shader::forceDebugSymbols(bool state) { debug = state; }

std::vector<uint32_t> Shader::compile(const std::string &source, const std::string &path, shaderc_shader_kind kind) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetTargetSpirv(shaderc_spirv_version_1_5);

    auto includerPtr = std::make_unique<ShaderUtils::ShaderIncluder>();
    for (auto &p : g_IncludePaths) {
        includerPtr->addIncludePath(p);
    }
    options.SetIncluder(std::move(includerPtr));

    // todo: add optimization level to config
    options.SetGenerateDebugInfo();
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, path.c_str(), m_entryPoint.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
}

std::string Shader::preprocess(const std::string &source) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    // Use the default includer for preprocessing (like your original code).

    auto includerPtr = std::make_unique<ShaderUtils::ShaderIncluder>();
    for (auto &p : g_IncludePaths) {
        includerPtr->addIncludePath(p);
    }
    options.SetIncluder(std::move(includerPtr));

    if (debug) {
        options.SetGenerateDebugInfo();
    }

    shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(source, m_kind, m_path.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
}

void Shader::reflect(const std::vector<uint32_t> &binary) {
    spv_reflect::ShaderModule reflecter = spv_reflect::ShaderModule(binary);
    if (reflecter.GetResult() != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to reflect shader module");
    }

    // Push constants
    std::vector<SpvReflectBlockVariable *> push = ShaderUtils::getPushConstants(reflecter);
    if (!push.empty()) {
        pushConstants = ShaderUtils::generatePushConstantLayout(*push[0]);
    }

    // Descriptor sets
    std::vector<SpvReflectDescriptorSet *> rawSignature = ShaderUtils::getDescriptorSets(reflecter);
    if (rawSignature.empty()) {
        return;
    }

    // Clear previous reflection data to avoid duplicating when re-reflecting
    for (auto &dsi : descriptorSetInfos) {
        dsi.setIndex = -1;
        dsi.bindings.clear();
    }
    uboLayouts.clear();

    for (auto &set : rawSignature) {
        std::vector<SpvReflectDescriptorBinding *> bindings(set->bindings, set->bindings + set->binding_count);
        for (SpvReflectDescriptorBinding *binding : bindings) {
            if (descriptorSetInfos[binding->set].setIndex == -1) {
                descriptorSetInfos[binding->set].setIndex = binding->set;
            }

            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                RHI::UBOLayout ubo = ShaderUtils::generateUBOLayout(*binding);
                descriptorSetInfos[binding->set].bindings.push_back({
                    {binding->binding, static_cast<vk::DescriptorType>(binding->descriptor_type), binding->count, m_stage, nullptr},
                    ubo.uboName
                });
                uboLayouts.push_back(ubo);
            } else {
                descriptorSetInfos[binding->set].bindings.push_back({
                    {binding->binding, static_cast<vk::DescriptorType>(binding->descriptor_type), binding->count, m_stage, nullptr},
                    ""
                });
            }
        }
    }
}

} // namespace Shader
