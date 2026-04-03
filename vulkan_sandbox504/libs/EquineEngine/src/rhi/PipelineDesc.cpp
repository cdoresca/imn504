#include "PipelineDesc.hpp"
#include "renderer/vulkan/RHIHelper.hpp"

namespace RHI {

void PipelineDesc::init(const std::vector<std::string> &shaderPaths) {
    this->shaderPaths = shaderPaths;
    uuid.generate();
    shaders.resize(shaderPaths.size());
    loadShaders();
}

void PipelineDesc::loadShaders() {
    reloadShaders();
    for (int i = 0; i < shaders.size(); i++) {
        // build pushConstantRanges
        // for now we enforce that the shaders have the same layout
        if (shaders[i].getPushConstants().paddedSize != 0) {
            if (pushConstants == nullptr) {
                pushConstants = new PushConstant(shaders[i].getPushConstants());
                pushConstants->stages |= shaders[i].getStage();
            } else {
                if (pushConstants->layout != shaders[i].getPushConstants()) {
                    throw std::runtime_error("Push constant layout mismatch");
                }
                pushConstants->stages |= shaders[i].getStage();
            }
        }

        // build descriptorSetsInfos
        auto tempDescriptorSetInfos = shaders[i].getDescriptorSetInfos();
        for (auto &setInfo : tempDescriptorSetInfos) {
            if (setInfo.setIndex == -1) {
                continue; // skip empty descriptor sets
            }
            Shader::DescriptorSetInfo &currentSet = descriptorSetLayouts[setInfo.setIndex];
            currentSet.setIndex = setInfo.setIndex;

            for (Shader::DescriptorBinding &binding : setInfo.bindings) {
                auto it = std::find_if(currentSet.bindings.begin(), currentSet.bindings.end(), [&binding](const Shader::DescriptorBinding &b) {
                    return b.binding.binding == binding.binding.binding;
                });
                if (it != currentSet.bindings.end()) {
                    if (it->binding.descriptorType != binding.binding.descriptorType) {
                        throw std::runtime_error("DescriptorSet type ambiguity");
                    }
                    it->binding.stageFlags |= binding.binding.stageFlags;
                } else {
                    currentSet.bindings.push_back(binding);
                }
            }
        }

        // prepare ManagedUBOs
        std::vector<UBOLayout> tempUbos = shaders[i].getUBOs();
        for (auto &layout : tempUbos) {
            if (auto findit = ubos.find(layout.uboName); findit != ubos.end()) { // if it exists already
                if (findit->second[0].layout.fields != layout.fields) {          // if the layout is different
                    throw std::runtime_error("UBO layout mismatch, only one name par layout is allowed");
                }
                // check if the member names are the same
                if (!std::equal(findit->second[0].layout.fieldIndexToNameMap.begin(), findit->second[0].layout.fieldIndexToNameMap.end(), layout.fieldIndexToNameMap.begin())) {
                    throw std::runtime_error("UBO member name mismatch, for merged UBOs, member names must be identical");
                }
            } else {
                ubos[layout.uboName].emplace_back(layout); // this should be the first element, always
            }
        }
    }
    // count the number of descriptor sets
    int previous = -1;
    for (int i = 0; i < descriptorSetLayouts.size(); i++) {
        if (descriptorSetLayouts[i].setIndex != -1) {
            if (descriptorSetLayouts[i].setIndex != previous + 1) {
                throw std::runtime_error("Descriptor set index gap");
            }
            descriptorSetCount++;
            previous = descriptorSetLayouts[i].setIndex;
        }
    }
}

void PipelineDesc::reloadShaders() {
    for (int i = 0; i < shaderPaths.size(); i++) {
        shaders[i].load(shaderPaths[i]);
        isDirty |= shaders[i].changed();
    }
}

std::vector<Shader::Shader *> PipelineDesc::getShaders() {
    std::vector<Shader::Shader *> result;
    for (auto &shader : shaders) {
        result.push_back(&shader);
    }
    return result;
}

std::vector<std::vector<ManagedUBO>> PipelineDesc::getUbos() const {
    std::vector<std::vector<ManagedUBO>> uboVec;
    if (!ubos.empty()) {
        for (auto &[name, ubo] : ubos) {
            uboVec.push_back(ubo);
        }
    }
    return uboVec;
}

void PipelineDesc::setUBO(BaseUBO *data, const std::string &uboName, uint variantIndex) {
    validateUboName(uboName);
    ubos[uboName][variantIndex].bindUboData(data);
}

void PipelineDesc::setPushConstant(BaseUBO *data) {
    // TODO: validate size;
    pushConstants->bindUboData(data);
}

void PipelineDesc::setVertexInput(const std::vector<VertexInputBinding> &bindings, const std::vector<VertexInputAttribute> &attributes) {
    m_vertexInputBindings.clear();
    m_vertexInputAttributes.clear();

    for (const auto& binding : bindings) {
        vk::VertexInputBindingDescription vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.stride = binding.stride;
        vkBinding.inputRate = vertexInputRateToVkVertexInputRate(binding.inputRate);
        m_vertexInputBindings.push_back(vkBinding);
    }

    for (const auto& attribute : attributes) {
        vk::VertexInputAttributeDescription vkAttribute{};
        vkAttribute.location = attribute.location;
        vkAttribute.binding = attribute.binding;
        vkAttribute.format = formatToVkFormat(attribute.format);
        vkAttribute.offset = attribute.offset;
        m_vertexInputAttributes.push_back(vkAttribute);
    }

    vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindings.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions = m_vertexInputBindings.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributes.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = m_vertexInputAttributes.data();
}

} // namespace RHI

