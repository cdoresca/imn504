#pragma once

#include "EngineConfig.hpp"
#include "shaderUtils.hpp"
#include <array>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>


namespace Shader {

extern std::vector<std::filesystem::path> g_IncludePaths; // defined in Shader.cpp

struct DescriptorBinding {
    vk::DescriptorSetLayoutBinding binding;
    std::string uboName;
};

struct DescriptorSetInfo {
    int setIndex = -1;
    std::vector<DescriptorBinding> bindings;
};

class Shader {
protected:
    size_t m_hash;
    std::string m_path;
    std::string m_source; // shader source code
    std::string m_entryPoint;

    shaderc_shader_kind m_kind;
    std::vector<uint32_t> m_Binary;
    vk::ShaderStageFlagBits m_stage;
    std::array<DescriptorSetInfo, EE::Config::kMaxDescriptorSets> descriptorSetInfos;
    std::vector<RHI::UBOLayout> uboLayouts;
    RHI::UBOLayout pushConstants;

    bool hasChanged;
    bool debug;

public:
    Shader();

    void load(const std::string &path, const std::string &entryPoint = "main");

    vk::ShaderStageFlagBits getStage() const;
    bool changed() const;
    std::string getPath() const;
    std::string getSource() const;
    std::vector<uint32_t> getBinary() const;
    std::array<DescriptorSetInfo, EE::Config::kMaxDescriptorSets> getDescriptorSetInfos() const;

    vk::ShaderModuleCreateInfo getShaderCreateInfo();
    vk::PipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo();

    std::vector<RHI::UBOLayout> getUBOs();
    RHI::UBOLayout getPushConstants();

    void forceDebugSymbols(bool state);

protected:
    std::vector<uint32_t> compile(const std::string &source, const std::string &path, shaderc_shader_kind kind);
    std::string preprocess(const std::string &source);
    void reflect(const std::vector<uint32_t> &binary);
};

} // namespace Shader
