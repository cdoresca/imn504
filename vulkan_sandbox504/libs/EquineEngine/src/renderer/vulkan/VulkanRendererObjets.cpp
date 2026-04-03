#include "RHIHelper.hpp"
#include "VkUtils.hpp"
#include "VulkanRenderer.hpp"
#include "rhi/PipelineDesc.hpp"
#include "shader/shader.hpp"

#include <imgui_impl_vulkan.h>

namespace VKRenderer {
void Renderer::createSwapChain() {
    // query swapchain support
    SwapChainDetails swapChainSupport = querySwapChainDetails(m_physicalDevice, m_surface);

    // choose swapchain settings
    vk::SurfaceFormatKHR swapchainSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR swapchainPresentMode = swapchainPresentMode = chooseSwapPresentMode(swapChainSupport.presentModes, m_vsync);
    vk::Extent2D swapchainExtent = chooseSwapExtent(swapChainSupport.surfaceCapabilities, m_window);
    // Ensure initial viewport/scissor size matches the actual swapchain extent before first frame
    m_windowSize = {static_cast<int>(swapchainExtent.width), static_cast<int>(swapchainExtent.height)};
    // TODO: must ensure same amount as in m_frameData
    /*
    uint32_t swapchainImageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
    if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 && swapchainImageCount > swapChainSupport.surfaceCapabilities.maxImageCount) { swapchainImageCount = swapChainSupport.surfaceCapabilities.maxImageCount; }*/
    uint32_t swapchainImageCount = EE::Config::kMaxFramesInFlight;

    // create swapchain
    vk::SwapchainCreateInfoKHR swapchainCreateInfo({}, m_surface, swapchainImageCount, swapchainSurfaceFormat.format, swapchainSurfaceFormat.colorSpace, swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);

    uint32_t queueFamilyIndices[] = {(uint32_t)m_queueFamilyIndices.graphicsQueueIndex, (uint32_t)m_queueFamilyIndices.presentQueueIndex};
    if (m_queueFamilyIndices.graphicsQueueIndex != m_queueFamilyIndices.presentQueueIndex) {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE; // BEWARE, this can affect effect that depend on neighboring pixels, if they are obscured by another window, they will be clipped, and thus the effect may not work for this region

    m_swapChain = m_logicalDevice.createSwapchainKHR(swapchainCreateInfo);
    std::vector<vk::Image> swapchainImages = m_logicalDevice.getSwapchainImagesKHR(m_swapChain);
    for (size_t i = 0; i < m_frameData.size(); ++i) {
        // m_frameData[i] = {};
        m_frameData[i].swapchainImage = {swapchainImages[i], nullptr, nullptr, swapchainSurfaceFormat.format, Vec3u(swapchainExtent.width, swapchainExtent.height, 1)};
        m_frameData[i].swapchainImage.externalOwned = true;
        m_frameData[i].swapchainImage.imageView = createImageView(m_frameData[i].swapchainImage.image, swapchainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor);
        m_frameData[i].depth = {nullptr, nullptr, nullptr, vk::Format::eD32Sfloat, Vec3u(swapchainExtent.width, swapchainExtent.height, 1)};
        auto [depthImage, depthMemory] = createImageInternal(vk::ImageType::e2D, m_frameData[i].depth.size, m_frameData[i].depth.format, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, "dephImage");
        m_frameData[i].depth.image = depthImage;
        m_frameData[i].depth.memory = depthMemory;
        m_frameData[i].depth.imageView = createImageView(m_frameData[i].depth.image, m_frameData[i].depth.format, vk::ImageAspectFlagBits::eDepth);
        // Allocate a stable UUID for this internal depth so RHI descriptors can alias it
        m_frameData[i].depth.uuidOverride.generate();
        m_textureRegistry[m_frameData[i].depth.uuidOverride] = m_frameData[i].depth;
    }
}

void Renderer::cleanupSwapChain() {
    m_logicalDevice.waitIdle();

    // cleanup textures
    // TODO: only cleanup RenderTargets

    // m_logicalDevice.freeCommandBuffers(m_graphicsCommandPool, m_commandBuffers);
    //  cleanup framebuffers

    for (FrameData &fb : m_frameData) {
        if (fb.swapchainImage.imageView) {
            m_logicalDevice.destroyImageView(fb.swapchainImage.imageView);
            fb.swapchainImage.imageView = nullptr;
        }
        if (fb.depth.imageView) {
            m_logicalDevice.destroyImageView(fb.depth.imageView);
            fb.depth.imageView = nullptr;
        }
        if (fb.depth.image) {
            m_logicalDevice.destroyImage(fb.depth.image);
            fb.depth.image = nullptr;
        }
        if (fb.depth.memory) {
            m_logicalDevice.freeMemory(fb.depth.memory);
            fb.depth.memory = nullptr;
        }
        // Keep UUID alias mapping consistent: erase depth UUID entry from registry
        if ((uint64_t)fb.depth.uuidOverride != INVALID_UUID) {
            m_textureRegistry.erase(fb.depth.uuidOverride);
            fb.depth.uuidOverride = INVALID_UUID;
        }
    }
    m_logicalDevice.destroySwapchainKHR(m_swapChain);
}

void Renderer::recreateSwapchain() {
    glfwGetFramebufferSize(m_window, &m_windowSize.x, &m_windowSize.y);
    /*Not the best way, we need to wait for all frames in flight to finish rendering before we recreate the swapChain,
     * by using the oldSwapChain field when building the swapChain, we should be able to rebuild without waiting
     * for the still rendering frame. Will need to research that.
     */
    cleanupSwapChain();
    createSwapChain();
}

void Renderer::createTransientCommandPool() {
    const vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, m_queueFamilyIndices.graphicsQueueIndex);
    m_transientPool = m_logicalDevice.createCommandPool(commandPoolCreateInfo);
}

void Renderer::createMainCommandBuffers() {
    const vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queueFamilyIndices.graphicsQueueIndex);
    for (int i = 0; i < m_frameData.size(); ++i) {
        m_frameData[i].frameNumber = i;
        m_frameData[i].commandPool = m_logicalDevice.createCommandPool(commandPoolCreateInfo);

        const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(m_frameData[i].commandPool, vk::CommandBufferLevel::ePrimary, 2u);
        std::vector<vk::CommandBuffer> commandBuffers = m_logicalDevice.allocateCommandBuffers(commandBufferAllocateInfo);
        m_frameData[i].commandBuffer = commandBuffers[0];
        m_frameData[i].uiCommandBuffer = commandBuffers[1];
    }
}

void Renderer::createDescriptorPool() {
    vk::DescriptorPoolSize poolSizeSSBO(vk::DescriptorType::eStorageBuffer, 100);
    vk::DescriptorPoolSize poolSizeSampledImage(vk::DescriptorType::eSampledImage, 100);
    vk::DescriptorPoolSize poolSizeStorageImage(vk::DescriptorType::eStorageImage, 100);
    vk::DescriptorPoolSize poolSizeUBO(vk::DescriptorType::eUniformBuffer, 100);
    vk::DescriptorPoolSize poolSizeSampler(vk::DescriptorType::eSampler, 100);

    constexpr uint poolCount = 5;
    vk::DescriptorPoolSize sizes[poolCount] = {poolSizeSSBO, poolSizeSampledImage, poolSizeStorageImage, poolSizeUBO, poolSizeSampler};
    vk::DescriptorPoolCreateInfo poolInfo({}, 1000, poolCount, sizes);
    m_descriptorPool = m_logicalDevice.createDescriptorPool(poolInfo);
}

void Renderer::createQueryPools() {
    vk::QueryPoolCreateInfo queryPoolCreateInfo({}, vk::QueryType::ePipelineStatistics, 1000, m_perfCounterGlobals.pipelineStatsFlags);
    uint count = 0;
    for (FrameData &frameData : m_frameData) {
        queryPoolCreateInfo.queryType = vk::QueryType::ePipelineStatistics;
        frameData.perfCounters.pipelineStatisticsQueryPool = m_logicalDevice.createQueryPool(queryPoolCreateInfo);
        queryPoolCreateInfo.queryType = vk::QueryType::eTimestamp;
        frameData.perfCounters.timestampQueryPool = m_logicalDevice.createQueryPool(queryPoolCreateInfo);
        // queryPoolCreateInfo.queryType = vk::QueryType::eMeshPrimitivesGeneratedEXT;
        // frameData.perfCounters.meshPrimitivesQueryPool = m_logicalDevice.createQueryPool(queryPoolCreateInfo);

        resetQueryPools(count);
        count++;
    }
}

void Renderer::createPipeline(RHI::PipelineDesc &pipelineDesc) {
    if (auto findit = m_pipelineRegistry.find(pipelineDesc.uuid); findit != m_pipelineRegistry.end()) { return; }
    pipelineDesc.setDirty(false);
    Pipeline pipeline;
    auto allshaders = pipelineDesc.getShaders();

    if (allshaders.size() > 0) {
        pipelineDesc.isCompute = allshaders[0]->getStage() == vk::ShaderStageFlagBits::eCompute;
    }

    uint descriptorSetCount = pipelineDesc.getDescriptorSetCount();

    auto descriptorSetInfos = pipelineDesc.getDescriptorSetInfos();
    pipeline.setLayouts.resize(descriptorSetCount);
    // descriptor must be created consecutively, so we are garanteed that the descriptor set index is the same as the index in the array and that there are no gaps
    for (int i = 0; i < descriptorSetCount; i++) {
        pipeline.descriptorBindingsCount += descriptorSetInfos[i].bindings.size();
        std::vector<vk::DescriptorSetLayoutBinding> localBindings(descriptorSetInfos[i].bindings.size());
        for (int j = 0; j < descriptorSetInfos[i].bindings.size(); j++) {
            localBindings[j] = descriptorSetInfos[i].bindings[j].binding;
        }
        pipeline.setLayouts[i] = m_logicalDevice.createDescriptorSetLayout({{}, static_cast<uint32_t>(localBindings.size()), localBindings.data()});
    }

    vk::PushConstantRange pushConstantRange;
    vk::PipelineLayoutCreateInfo PipelineLayoutInfo({}, pipeline.setLayouts.size(), pipeline.setLayouts.data());
    if (pipelineDesc.getPushConstants() != nullptr) {
        // check push constant size < maxPushConstantsSize
        pushConstantRange = vk::PushConstantRange(pipelineDesc.getPushConstants()->stages, 0, pipelineDesc.getPushConstants()->size);
        PipelineLayoutInfo.pushConstantRangeCount = 1;
        PipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }
    pipeline.layout = m_logicalDevice.createPipelineLayout(PipelineLayoutInfo);
    {
        std::vector<vk::ShaderModule> tempShaderModules;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        tempShaderModules.resize(allshaders.size());
        shaderStages.resize(allshaders.size());

        for (int i = 0; i < allshaders.size(); i++) {
            shaderStages[i] = allshaders[i]->getPipelineShaderStageCreateInfo();
            tempShaderModules[i] = m_logicalDevice.createShaderModule(allshaders[i]->getShaderCreateInfo());
            shaderStages[i].module = tempShaderModules[i];
        }

        if (pipelineDesc.isCompute) {
            vk::ComputePipelineCreateInfo pipelineCreateInfo({}, shaderStages[0], pipeline.layout);

            pipeline.pipeline = m_logicalDevice.createComputePipeline(VK_NULL_HANDLE, pipelineCreateInfo).value;
        } else {
            // New create info to define color, depth and stencil attachments at pipeline create time
            vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{{}, 1, &m_frameData[0].swapchainImage.format, m_frameData[0].depth.format};

            // create pipeline
            vk::GraphicsPipelineCreateInfo pipelineCreateInfo({}, shaderStages.size(), shaderStages.data(),
                                                              &pipelineDesc.vertexInputStateCreateInfo,
                                                              &pipelineDesc.inputAssemblyStateCreateInfo,
                                                              nullptr,
                                                              &pipelineDesc.viewportStateCreateInfo,
                                                              &pipelineDesc.rasterizationStateCreateInfo,
                                                              &pipelineDesc.multisampleStateCreateInfo,
                                                              &pipelineDesc.depthStencilStateCreateInfo,
                                                              &pipelineDesc.colorBlendStateCreateInfo,
                                                              &pipelineDesc.dynamicStateCreateInfo,
                                                              pipeline.layout);
            pipelineCreateInfo.setPNext(&pipelineRenderingCreateInfo);
            // Create the pipeline, we discard the result code
            pipeline.pipeline = m_logicalDevice.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCreateInfo).value;
        }

        for (auto shader : tempShaderModules) {
            m_logicalDevice.destroyShaderModule(shader);
        }
    }
    pipeline.pipelineData = &pipelineDesc;
    m_pipelineRegistry[pipelineDesc.uuid] = pipeline;
}

void Renderer::createBuffer(RHI::BufferDesc &bufferDesc) {
    // todo: check if buffer already exists
    if (bufferDesc.size == 0) { throw std::runtime_error("Buffer size cannot be 0"); }
    Buffer buffer;
    auto [vkBuffer, vkMemory] = createBufferInternal(bufferDesc.size, bufferDescToVkBufferUsage(bufferDesc), vk::MemoryPropertyFlagBits::eDeviceLocal, bufferDesc.debugName);
    buffer.buffer = vkBuffer;
    buffer.memory = vkMemory;
    m_bufferRegistry[bufferDesc.uuid] = buffer;
}

void Renderer::createTexture(RHI::TextureDesc &textureDesc) {
    // TODO: enforce texture size
    Texture texture;
    texture.size = Vec3u(textureDesc.width, textureDesc.height, textureDesc.depth);
    texture.format = formatToVkFormat(textureDesc.format);
    // Beware we do not support multisampling for now
    auto [vkImage, vkMemory] = createImageInternal(textureDimToVkImageType(textureDesc.dim), texture.size, texture.format, textureDescToVkImageUsage(textureDesc), vk::MemoryPropertyFlagBits::eDeviceLocal, textureDesc.debugName, textureDesc.mipLevels, textureDesc.arrayLayers);
    texture.image = vkImage;
    texture.memory = vkMemory;
    texture.imageView = createImageView(texture.image, texture.format, vk::ImageAspectFlagBits::eColor, textureDimToVkImageViewType(textureDesc.dim));
    if (textureDesc.isImguiReady) {
        texture.hasImGui = true;
        texture.ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(m_samplerRegistry[gDefaultLinearSampler], texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (textureDesc.isStorage) {
        transitionImageToRead(texture);
    }

    m_textureRegistry[textureDesc.uuid] = texture;
}

ImTextureID Renderer::getImGuiCompatibleTextureHandle(RHI::TextureDesc &textureDesc) {
    if (auto findit = m_textureRegistry.find(textureDesc.uuid); findit != m_textureRegistry.end()) {
        if (findit->second.hasImGui) {
            return (ImTextureID)findit->second.ImGuiDescriptorSet;
        } else {
            throw std::runtime_error("Texture is not ImGui compatible");
        }
    }
    throw std::runtime_error("Texture does not exist");
}

void Renderer::createSampler(RHI::SamplerDesc &samplerDesc) {
    if (auto findit = m_samplerRegistry.find(samplerDesc.uuid); findit != m_samplerRegistry.end()) { return; }
    vk::SamplerCreateInfo samplerCreateInfo({}, filterModeToVkFilter(samplerDesc.magFilter), filterModeToVkFilter(samplerDesc.minFilter), mipMapModeToVkSamplerMipmapMode(samplerDesc.mipFilter), addressModeToVkSamplerAddressMode(samplerDesc.addressModeU), addressModeToVkSamplerAddressMode(samplerDesc.addressModeV), addressModeToVkSamplerAddressMode(samplerDesc.addressModeW), samplerDesc.mipLodBias, (samplerDesc.maxAnisotropy != 0), samplerDesc.maxAnisotropy, false, vk::CompareOp::eNever, 0.f, std::numeric_limits<float>::max(), vk::BorderColor::eFloatOpaqueBlack, false);
    m_samplerRegistry[samplerDesc.uuid] = m_logicalDevice.createSampler(samplerCreateInfo);
}

vk::ImageView Renderer::createImageView(vk::Image p_image, vk::Format p_format, vk::ImageAspectFlags p_aspectFlags, vk::ImageViewType p_viewType) {
    vk::ImageViewCreateInfo viewInfo({}, p_image, p_viewType, p_format, {}, {p_aspectFlags, 0, 1, 0, 1});
    vk::ImageView imageView = m_logicalDevice.createImageView(viewInfo, nullptr);
    return imageView;
}
} // namespace VKRenderer