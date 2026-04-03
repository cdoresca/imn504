#include "VkUtils.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanRendererDefs.hpp"
#include "rhi/CommandList.hpp"
#include "shader/shader.hpp"
#include <filesystem>
#include <iostream>


namespace VKRenderer {

constexpr uint uploadChunkSize = 128 * 1024 * 1024;   // 128MB
constexpr uint downloadChunkSize = 128 * 1024 * 1024; // 128MB

// Utils
bool Renderer::isDeviceSuitable(vk::PhysicalDevice device) {
    // query device properties and features
    // TODO: check the struct to verify features are supported
    auto const &[features, featuresMeshEXT, featuresMaintenance5] = device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceMeshShaderFeaturesEXT, vk::PhysicalDeviceMaintenance5FeaturesKHR>();
    auto const &[prop, propMeshEXT, propMaintenance5] = device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceMeshShaderPropertiesEXT, vk::PhysicalDeviceMaintenance5PropertiesKHR>();

    std::string deviceName = prop.properties.deviceName;
    std::cout << "Device " << deviceName << " is being evaluated." << std::endl;

    // check for queue support
    QueueFamilyIndices deviceFamilyIndices = queryQueueFamilies(device, m_surface);
    if (!deviceFamilyIndices.isComplete()) {
        std::cout << "Device " << deviceName << " does not have the required queues." << std::endl;
        return false;
    }
    m_queueFamilyIndices = deviceFamilyIndices;

    // check for extensions support
    // Merge core + optional to validate support
    auto mergedExts = EE::Config::mergeExtensions(m_optionalDeviceExtensions);
    if (!checkDeviceExtensionSupport(device, mergedExts)) {
        std::cout << "Device " << deviceName << " does not have the required extensions." << std::endl;
        return false;
    }

    // check for swapchain support
    // TODO: verify this is a valid verification
    SwapChainDetails swapChainSupport = querySwapChainDetails(device, m_surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        std::cout << "Device " << deviceName << " does not have the required swapchain support." << std::endl;
        return false;
    }

    // TODO: check for other like mesh shaders ...

    std::cout << "Device " << deviceName << " is suitable." << std::endl;
    return true;
}

vk::CommandBuffer Renderer::beginSingleTimeCommands(vk::CommandPool &p_commandPool) {
    vk::CommandBufferAllocateInfo allocInfo(p_commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer commandBuffer = m_logicalDevice.allocateCommandBuffers(allocInfo)[0];
    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    return commandBuffer;
}

void Renderer::endSingleTimeCommands(vk::CommandPool &p_commandPool, vk::CommandBuffer p_commandBuffer) {
    p_commandBuffer.end();
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &p_commandBuffer);
    vk::Result submitResult = m_graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE);
    if (submitResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to submit command buffer.");
    }
    m_graphicsQueue.waitIdle();
    m_logicalDevice.freeCommandBuffers(p_commandPool, 1, &p_commandBuffer);
}

// 0: timestampQueryIndex
uint PerfCounterGlobals::generateTimestampCLParams(Pipeline &pipeline, std::vector<RHI::CommandParameter *> &parameters) {
    uint index = parameters.size();                                       // to return
    parameters.push_back(new RHI::CommandParameter(timestampQueryIndex)); // only store start, end is start + 1
    pipeline.instrumentationInfo.timerIDs.emplace_back(timestampQueryIndex);
    timestampQueryIndex += 2;
    return index;
}
// 0: pipelineStatsQueryIndex, 1: meshPrimitivesQueryIndex
uint PerfCounterGlobals::generatePerfCounterCLParamsWMeshShader(Pipeline &pipeline, std::vector<RHI::CommandParameter *> &parameters) {
    uint index = parameters.size(); // to return
    parameters.push_back(new RHI::CommandParameter(pipelineStatisticsQueryIndex));
    pipeline.instrumentationInfo.pipelineStatsIDs.emplace_back(pipelineStatisticsQueryIndex);
    pipelineStatisticsQueryIndex++;
    parameters.push_back(new RHI::CommandParameter(meshPrimitivesQueryIndex));
    pipeline.instrumentationInfo.meshPrimIDs.emplace_back(meshPrimitivesQueryIndex);
    meshPrimitivesQueryIndex++;
    return index;
}

// Objects Utils
std::pair<vk::Buffer, vk::DeviceMemory> Renderer::createBufferInternal(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, std::string debugName) {
    // Create the buffer
    vk::BufferCreateInfo bufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive);
    vk::Buffer buffer = m_logicalDevice.createBuffer(bufferCreateInfo);
    vk::StructureChain<vk::MemoryRequirements2> memRequirements = m_logicalDevice.getBufferMemoryRequirements2(vk::BufferMemoryRequirementsInfo2(buffer));
    vk::MemoryAllocateInfo allocInfo(memRequirements.get<vk::MemoryRequirements2>().memoryRequirements.size, findMemoryType(memRequirements.get<vk::MemoryRequirements2>().memoryRequirements.memoryTypeBits, properties));
    vk::DeviceMemory bufferMemory = m_logicalDevice.allocateMemory(allocInfo);
    m_logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
    if (!debugName.empty()) { m_logicalDevice.setDebugUtilsObjectNameEXT({vk::ObjectType::eBuffer, (uint64_t)(VkBuffer)buffer, debugName.c_str()}, VULKAN_HPP_DEFAULT_DISPATCHER); }
    return {buffer, bufferMemory};
}

std::pair<vk::Image, vk::DeviceMemory> Renderer::createImageInternal(vk::ImageType dim, Vec3u size, vk::Format format, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, std::string debugName, uint mipLevel, uint arrayCount, vk::SampleCountFlagBits sample, vk::ImageTiling tiling) {
    vk::ImageCreateInfo imageCreateInfo({}, dim, format, vk::Extent3D(size.x, size.y, size.z), mipLevel, arrayCount, sample, tiling, usage, vk::SharingMode::eExclusive);
    vk::Image image = m_logicalDevice.createImage(imageCreateInfo);
    vk::StructureChain<vk::MemoryRequirements2> memRequirements = m_logicalDevice.getImageMemoryRequirements2(vk::ImageMemoryRequirementsInfo2(image));
    vk::MemoryAllocateInfo allocInfo(memRequirements.get<vk::MemoryRequirements2>().memoryRequirements.size, findMemoryType(memRequirements.get<vk::MemoryRequirements2>().memoryRequirements.memoryTypeBits, properties));
    vk::DeviceMemory imageMemory = m_logicalDevice.allocateMemory(allocInfo);
    m_logicalDevice.bindImageMemory(image, imageMemory, 0);
    if (!debugName.empty()) { m_logicalDevice.setDebugUtilsObjectNameEXT({vk::ObjectType::eImage, (uint64_t)(VkImage)image, debugName.c_str()}, VULKAN_HPP_DEFAULT_DISPATCHER); }
    return {image, imageMemory};
}

// Memory Utils
uint32_t Renderer::findMemoryType(uint32_t p_typeFilter, vk::MemoryPropertyFlags p_properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    m_physicalDevice.getMemoryProperties(&memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((p_typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & p_properties) == p_properties) { return i; }
    }
    throw std::runtime_error("Failed to find suitable memory type.");
}

void Renderer::copyBuffer(const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, uint size, uint srcOffset, uint dstOffset) {
    // We could create a command pool dedicated to memory transfert, but for now interlacing memory transfert commands
    // between render commands is fine as our application transfers data rarely.
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(m_transientPool);
    vk::BufferCopy2 copyRegion(srcOffset, dstOffset, size);
    commandBuffer.copyBuffer2({srcBuffer, dstBuffer, 1, &copyRegion});
    endSingleTimeCommands(m_transientPool, commandBuffer);
}

void Renderer::uploadDataToBufferInternal(const void *data, const uint size, const vk::Buffer dstBuffer, const uint offset) {
    const uint64 stagingCapacity = std::min(static_cast<uint64>(size), static_cast<uint64>(uploadChunkSize));
    auto [stagingBuffer, stagingMemory] = createBufferInternal(stagingCapacity, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *stagingPtr = m_logicalDevice.mapMemory(stagingMemory, 0, stagingCapacity);
    const uint8 *srcPtr = static_cast<const uint8 *>(data);
    uint64 srcOffset;
    for (srcOffset = 0; srcOffset + uploadChunkSize <= size; srcOffset += uploadChunkSize) {
        memcpy(stagingPtr, srcPtr + srcOffset, uploadChunkSize);
        copyBuffer(stagingBuffer, dstBuffer, uploadChunkSize, 0, static_cast<uint>(offset + srcOffset));
    }

    const uint64 remainder = size - srcOffset;
    if (remainder > 0) {
        memcpy(stagingPtr, srcPtr + srcOffset, remainder);
        copyBuffer(stagingBuffer, dstBuffer, static_cast<uint>(remainder), 0, static_cast<uint>(offset + srcOffset));
    }

    m_logicalDevice.unmapMemory(stagingMemory);
    m_logicalDevice.destroyBuffer(stagingBuffer);
    m_logicalDevice.freeMemory(stagingMemory);
}

void Renderer::downloadDataFromBufferInternal(void *dst, const uint size, const vk::Buffer srcBuffer, const uint offset) {
    const uint64 stagingCapacity = std::min(size, downloadChunkSize);
    auto [stagingBuffer, stagingMemory] = createBufferInternal(stagingCapacity, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached);

    void *stagingPtr = m_logicalDevice.mapMemory(stagingMemory, 0, stagingCapacity);
    uint8 *dstPtr = static_cast<uint8 *>(dst);
    uint64 srcOffset;
    for (srcOffset = 0; srcOffset + downloadChunkSize <= size; srcOffset += downloadChunkSize) {
        copyBuffer(srcBuffer, stagingBuffer, downloadChunkSize, static_cast<uint>(offset + srcOffset), 0);
        memcpy(dstPtr + srcOffset, stagingPtr, downloadChunkSize);
    }

    const uint64 remainder = size - srcOffset;
    if (remainder > 0) {
        copyBuffer(srcBuffer, stagingBuffer, static_cast<uint>(remainder), static_cast<uint>(offset + srcOffset), 0);
        memcpy(dstPtr + srcOffset, stagingPtr, remainder);
    }

    m_logicalDevice.unmapMemory(stagingMemory);
    m_logicalDevice.destroyBuffer(stagingBuffer);
    m_logicalDevice.freeMemory(stagingMemory);
}

void Renderer::setBufferInternal(const uint8 value, const uint size, const vk::Buffer dstBuffer, const uint offset) {
    const uint64 stagingCapacity = std::min(size, uploadChunkSize);
    auto [stagingBuffer, stagingMemory] = createBufferInternal(stagingCapacity, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *stagingPtr = m_logicalDevice.mapMemory(stagingMemory, 0, stagingCapacity);

    // Since the value is constant, we memset the entire staging buffer once here.
    memset(stagingPtr, value, stagingCapacity);

    uint64 dstOffset;
    for (dstOffset = 0; dstOffset + uploadChunkSize <= size; dstOffset += uploadChunkSize) {
        copyBuffer(stagingBuffer, dstBuffer, uploadChunkSize, 0, static_cast<uint>(offset + dstOffset));
    }

    const uint64 remainder = size - dstOffset;
    if (remainder > 0) {
        copyBuffer(stagingBuffer, dstBuffer, static_cast<uint>(remainder), 0, static_cast<uint>(offset + dstOffset));
    }

    m_logicalDevice.unmapMemory(stagingMemory);
    m_logicalDevice.destroyBuffer(stagingBuffer);
    m_logicalDevice.freeMemory(stagingMemory);
}

// buffer size could be computed, we could specify for each defined format,
// here we rely on the caller to provide a vector of a compatible type
void Renderer::uploadDataToImageInternal(const void *data, const uint size, const Texture &dstImage, const uint offset) {
    const uint64 stagingCapacity = std::min(size, uploadChunkSize);
    auto [stagingBuffer, stagingMemory] = createBufferInternal(stagingCapacity, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *stagingPtr = m_logicalDevice.mapMemory(stagingMemory, 0, stagingCapacity);
    const uint8 *srcPtr = static_cast<const uint8 *>(data);
    uint64 srcOffset;
    for (srcOffset = 0; srcOffset + uploadChunkSize <= size; srcOffset += uploadChunkSize) {
        memcpy(stagingPtr, srcPtr + srcOffset, uploadChunkSize);
        copyBuffertoImage(stagingBuffer, dstImage, uploadChunkSize, 0, static_cast<uint>(offset + srcOffset));
    }

    const uint64 remainder = size - srcOffset;
    if (remainder > 0) {
        memcpy(stagingPtr, srcPtr + srcOffset, remainder);
        copyBuffertoImage(stagingBuffer, dstImage, static_cast<uint>(remainder), 0, static_cast<uint>(offset + srcOffset));
    }

    m_logicalDevice.unmapMemory(stagingMemory);
    m_logicalDevice.destroyBuffer(stagingBuffer);
    m_logicalDevice.freeMemory(stagingMemory);
}

void Renderer::copyBuffertoImage(const vk::Buffer &srcBuffer, const Texture &dstImage, uint size, uint srcOffset, uint dstOffset) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(m_transientPool);
    vk::BufferImageCopy2 copyRegion(0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {0, 0, 0}, {dstImage.size.x, dstImage.size.y, dstImage.size.z});
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {vk::ImageMemoryBarrier({}, {}, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, dstImage.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})});
    commandBuffer.copyBufferToImage2({srcBuffer, dstImage.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion});
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, {vk::ImageMemoryBarrier({}, {}, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, dstImage.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})});
    endSingleTimeCommands(m_transientPool, commandBuffer);
}

void Renderer::transitionImageToRead(const Texture &image) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(m_transientPool);
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, {vk::ImageMemoryBarrier({}, {}, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})});
    endSingleTimeCommands(m_transientPool, commandBuffer);
}

void Renderer::addShaderIncludePaths(const std::filesystem::path &dir) {
    Shader::g_IncludePaths.push_back(dir);
}

void Renderer::removeShaderIncludePaths(const std::filesystem::path &dir) {
    std::filesystem::path normalized = std::filesystem::absolute(dir).lexically_normal();
    auto it = std::find(Shader::g_IncludePaths.begin(), Shader::g_IncludePaths.end(), normalized);
    if (it == Shader::g_IncludePaths.end()) return;
    Shader::g_IncludePaths.erase(it);
}

} // namespace VKRenderer