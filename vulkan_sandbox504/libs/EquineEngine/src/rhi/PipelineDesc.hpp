#pragma once
#include "EngineConfig.hpp"
#include "UUID.hpp"
#include "shader/managedUBO.hpp"
#include "shader/shader.hpp"
#include "rhi/RHIdefines.hpp"

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI {

class PipelineDesc {
private:
    // constant in the lifetime of the application
    std::array<Shader::DescriptorSetInfo, EE::Config::kMaxDescriptorSets> descriptorSetLayouts;
    uint descriptorSetCount = 0;
    std::unordered_map<std::string, std::vector<ManagedUBO>> ubos; // what to do about same name for different ubo
    PushConstant *pushConstants = nullptr;                         // for now we enforce that the shaders have the same layout

    // variable in the lifetime of the application
    std::vector<std::string> shaderPaths;
    std::vector<Shader::Shader> shaders;

    std::vector<vk::VertexInputBindingDescription> m_vertexInputBindings;
    std::vector<vk::VertexInputAttributeDescription> m_vertexInputAttributes;

private:
    void validateUboName(const std::string &name) { // this could be called only when in debug mode to save on iteration
        if (ubos.find(name) == ubos.end()) {
            throw std::runtime_error("UBO with name " + name + " not found");
        }
    }

public:
    void init(const std::vector<std::string> &shaderPaths);
    void setDirty(bool dirty) { isDirty = dirty; }
    PushConstant *getPushConstants() { return pushConstants; }

    UUID uuid = INVALID_UUID;
    bool hasTiming = false;
    bool hasFineInstrumentation = false;
    bool isDirty = true;
    // hardcoded like an animal for now
    vk::Viewport viewport{0.0f, 0.0f, 800, 600, 0.0f, 1.0f};
    vk::Rect2D scissor{
        {0,   0  },
        {800, 600}
    };
    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{{}, 1, &viewport, 1, &scissor};
    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{{}, vk::SampleCountFlagBits::e1, VK_FALSE};
    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f};
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{{}, 0, nullptr, 0, nullptr};
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{{}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f};
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{VK_FALSE, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
        {},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachmentState,
        {0.0f, 0.0f, 0.0f, 0.0f}
    };

    // When true, commands using this pipeline are recorded in a post-process pass after the main scene rendering
    bool isPostProcess = false;
    bool isCompute = false;

    vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{{}, 2, dynamicStates};

    void loadShaders();
    void reloadShaders();
    std::vector<Shader::Shader *> getShaders();
    int getShaderCount() const { return shaders.size(); }
    uint getDescriptorSetCount() const { return descriptorSetCount; }
    std::array<Shader::DescriptorSetInfo, EE::Config::kMaxDescriptorSets> getDescriptorSetInfos() const { return descriptorSetLayouts; }
    std::vector<std::vector<ManagedUBO>> getUbos() const;

    // UBO
    void setUBO(BaseUBO *data, const std::string &uboName, uint variantIndex = 0);

    // Push Constants
    void setPushConstant(BaseUBO *data);

    void setVertexInput(const std::vector<VertexInputBinding> &bindings, const std::vector<VertexInputAttribute> &attributes);
};
} // namespace RHI

