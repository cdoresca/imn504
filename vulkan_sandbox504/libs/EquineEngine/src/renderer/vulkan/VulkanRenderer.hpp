#pragma once
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "EngineConfig.hpp"
#include <filesystem>
#include <iostream>
#include <list>

#include "UUID.hpp"
#include "VulkanRendererDefs.hpp"
#include "rhi/ResourceDesc.hpp"

#include "defines.hpp"
#include "imgui.h"
#include "rhi/CommandList.hpp"

struct GLFWwindow;
namespace RHI {
struct PipelineStatistics;
struct CommandListIR;
struct CommandList;
class PipelineDesc;
struct UseResourceCommand;
struct BindPipelineCommand;
struct UseSamplerCommand;
struct UseVertexBufferCommand;
struct UseIndexBufferCommand;
struct RHICommand;
} // namespace RHI

namespace VKRenderer {
class Renderer {
public:
    void setOptionalDeviceExtensions(const std::vector<const char *> &exts) { m_optionalDeviceExtensions = exts; }

private:
    // Managed objects
    std::unordered_map<UUID, Pipeline> m_pipelineRegistry;
    std::unordered_map<UUID, Buffer> m_bufferRegistry;
    std::unordered_map<UUID, Texture> m_textureRegistry;
    std::unordered_map<UUID, vk::Sampler> m_samplerRegistry;

    // Vulkan objects
    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_logicalDevice;
    vk::SurfaceKHR m_surface;
    vk::Queue m_graphicsQueue;
    // vk::Queue m_computeQueue;
    vk::Queue m_presentQueue;
    vk::CommandPool m_transientPool;
    vk::SwapchainKHR m_swapChain;
    QueueFamilyIndices m_queueFamilyIndices;
    vk::DebugUtilsMessengerEXT m_debugMessenger;
    vk::PhysicalDeviceLimits m_deviceLimits;

    std::array<MainSyncObject, EE::Config::kMaxFramesInFlight> m_mainSyncObjects; // one per frame in flight
    std::array<FrameData, EE::Config::kMaxFramesInFlight> m_frameData;            // one per frame in flight
    std::vector<const char *> m_optionalDeviceExtensions;

    RHI::CommandListIR m_intermediateCommandList;
    uint m_currentFrame = 0;
    int m_vsync = 0;
    uint m_acquireImageIndex = 0;
    GLFWwindow *m_window = nullptr;
    Vec2i m_windowSize;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorPool m_imguiDescriptorPool;
    PerfCounterGlobals m_perfCounterGlobals;

    UUID gDefaultLinearSampler;

public:
    void compileAndUseCommandList(RHI::CommandList &commandList);
    RHI::PipelineStatistics getPipelineStatistics(const RHI::PipelineDesc &pipelineDesc);

    void createPipeline(RHI::PipelineDesc &pipelineDesc);
    void createBuffer(RHI::BufferDesc &bufferDesc);
    void createTexture(RHI::TextureDesc &textureDesc);
    ImTextureID getImGuiCompatibleTextureHandle(RHI::TextureDesc &textureDesc);
    void createSampler(RHI::SamplerDesc &samplerDesc);
    void createDefaultRessources();

    // Expose current frame default depth as a sampled texture handle for post-process passes
    RHI::TextureDesc getCurrentDepthTextureDesc();
    RHI::SamplerDesc getDefaultLinearSamplerDesc();

    void init(GLFWwindow *window, bool vSync);
    void setVSync(int syncType);
    void ImGuiNewFrame();
    void render(bool withUI = true);
    void present();
    void waitIdle();
    void cleanup();

    template <typename T>
    void uploadToBuffer(RHI::BufferDesc const &bufferDesc, const std::vector<T> data, unsigned offset = 0) {
        if (bufferDesc.size == 0) {
            throw std::runtime_error("Buffer size cannot be 0");
        }
        uint size = data.size() * sizeof(T);
        // TODO: check offset is compatible with minOffsetAlignment
        if (size > bufferDesc.size || offset + size > bufferDesc.size) {
            throw std::runtime_error("Invalid size or offset");
        }
        uploadDataToBufferInternal(data.data(), size, m_bufferRegistry[bufferDesc.uuid].buffer, offset);
    }

    void uploadToBuffer(RHI::BufferDesc const &bufferDesc, const void *data, uint size = 0, uint offset = 0) {
        if (bufferDesc.size == 0) {
            throw std::runtime_error("Buffer size cannot be 0");
        }
        // TODO: check offset is compatible with minOffsetAlignment
        if (size > bufferDesc.size || offset + size > bufferDesc.size) {
            throw std::runtime_error("Invalid size or offset");
        }
        if (size == 0) { // whole buffer
            size = bufferDesc.size;
        }
        uploadDataToBufferInternal(data, size, m_bufferRegistry[bufferDesc.uuid].buffer, offset);
    }

    void uploadToTexture(RHI::TextureDesc const &textureDesc, const void *data, uint size) { // for now we dont support subresource upload
        // todo: check size using format
        uploadDataToImageInternal(data, size, m_textureRegistry[textureDesc.uuid], 0);
    }

    template <typename T>
    std::vector<T> downloadFromBuffer(RHI::BufferDesc const &bufferDesc, unsigned offset = 0) {
        if (bufferDesc.size == 0) {
            throw std::runtime_error("Buffer size cannot be 0");
        }
        if (offset >= bufferDesc.size) {
            throw std::runtime_error("Offset out of range");
        }

        // bytes to read from offset to end
        uint bytes = bufferDesc.size - offset;

        // ensure buffer length is divisible by T
        if (bytes % sizeof(T) != 0) {
            throw std::runtime_error("Buffer size is not a multiple of element size");
        }

        uint count = bytes / sizeof(T);
        std::vector<T> out(count);
        std::cout << bufferDesc.size << std::endl;
        if (count > 0) {
            downloadDataFromBufferInternal(out.data(), bytes, m_bufferRegistry[bufferDesc.uuid].buffer, offset);
        }
        return out;
    }

    void downloadFromBuffer(RHI::BufferDesc const &bufferDesc, void *dst, uint size = 0, uint offset = 0) {
        if (bufferDesc.size == 0) {
            throw std::runtime_error("Buffer size cannot be 0");
        }
        if (offset >= bufferDesc.size) {
            throw std::runtime_error("Offset out of range");
        }

        if (size == 0) { // read whole buffer from offset
            size = bufferDesc.size - offset;
        }

        // TODO: check offset is compatible with minOffsetAlignment if needed
        if (size > bufferDesc.size || offset + size > bufferDesc.size) {
            throw std::runtime_error("Invalid size or offset");
        }

        downloadDataFromBufferInternal(dst, size, m_bufferRegistry[bufferDesc.uuid].buffer, offset);
    }

    void setBuffer(RHI::BufferDesc const &bufferDesc, uint8 value, uint size = 0, uint offset = 0) {
        if (bufferDesc.size == 0) {
            throw std::runtime_error("Buffer size cannot be 0");
        }
        if (offset >= bufferDesc.size) {
            throw std::runtime_error("Offset out of range");
        }

        if (size == 0) { // set whole buffer from offset
            size = bufferDesc.size - offset;
        }

        // TODO: check offset is compatible with minOffsetAlignment if needed
        if (size > bufferDesc.size || offset + size > bufferDesc.size) {
            throw std::runtime_error("Invalid size or offset");
        }

        setBufferInternal(value, size, m_bufferRegistry[bufferDesc.uuid].buffer, offset);
    }

    // shader stuff
    void addShaderIncludePaths(const std::filesystem::path &dir);
    void removeShaderIncludePaths(const std::filesystem::path &dir);

private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createDevice();
    void createSwapChain();
    void cleanupSwapChain();
    void recreateSwapchain();
    void createMainSyncObjects();
    void createTransientCommandPool();
    void createMainCommandBuffers();
    void createDescriptorPool();
    void createQueryPools();

    std::pair<vk::Buffer, vk::DeviceMemory> createBufferInternal(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, std::string debugName = "");
    std::pair<vk::Image, vk::DeviceMemory> createImageInternal(vk::ImageType dim, Vec3u size, vk::Format format, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, std::string debugName = "", uint mipLevel = 1, uint arrayCount = 1, vk::SampleCountFlagBits sample = vk::SampleCountFlagBits::e1, vk::ImageTiling tiling = vk::ImageTiling::eOptimal);
    void uploadDataToBufferInternal(const void *data, uint size, vk::Buffer dstBuffer, uint offset = 0);
    void downloadDataFromBufferInternal(void *dst, uint size, vk::Buffer srcBuffer, uint offset = 0);
    void setBufferInternal(uint8 value, uint size, vk::Buffer dstBuffer, uint offset);
    // buffer size could be computed, we could specify for each defined format,
    // here we rely on the caller to provide a vector of a compatible type
    void uploadDataToImageInternal(const void *data, uint size, const Texture &dstImage, uint offset);
    vk::ImageView createImageView(vk::Image p_image, vk::Format p_format, vk::ImageAspectFlags p_aspectFlags, vk::ImageViewType p_viewType = vk::ImageViewType::e2D);

    void updateUBOs(uint imageIndex);
    void updatePushConstants(uint imageIndex);

    void resetQueryPools(uint imageIndex);
    void makeQueryResultsAvailable(uint imageIndex);

    void initImgui();
    void cleanupImgui();
    void imguiCreateDescriptorPool();
    void renderUI(uint imageIndex);

    // Utils
    bool isDeviceSuitable(vk::PhysicalDevice device);
    vk::CommandBuffer beginSingleTimeCommands(vk::CommandPool &p_commandPool);
    void endSingleTimeCommands(vk::CommandPool &p_commandPool, vk::CommandBuffer p_commandBuffer);

    // Memory Utils
    uint32_t findMemoryType(uint32_t p_typeFilter, vk::MemoryPropertyFlags p_properties);
    void copyToDeviceMem(vk::DeviceMemory bufferMemory, void *src, uint size, uint offset = 0);
    void copyFromDeviceMem(vk::DeviceMemory bufferMemory, void *dst, uint size, uint offset = 0);
    void copyBuffer(const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, uint size, uint srcOffset = 0, uint dstOffset = 0);
    void copyBuffertoImage(const vk::Buffer &srcBuffer, const Texture &dstImage, uint size, uint srcOffset = 0, uint dstOffset = 0);
    void transitionImageToRead(const Texture &image);
    // commandList resolution
    void clearCommandList(RHI::CommandList &newCommandList);
    void resolveUseResourceCommand(RHI::UseResourceCommand *command, std::vector<vk::WriteDescriptorSet> &descriptorWrites, std::list<vk::DescriptorBufferInfo> &TempBufferInfos, std::list<vk::DescriptorImageInfo> &TempImageInfos);
    void resolveUseVertexBuffer(RHI::CommandListIR &ir, RHI::UseVertexBufferCommand *command);
    void resolveUseIndexBuffer(RHI::CommandListIR &ir, RHI::UseIndexBufferCommand *command);
    void resolveUseSamplerCommand(RHI::UseSamplerCommand *command, std::vector<vk::WriteDescriptorSet> &descriptorWrites, std::list<vk::DescriptorImageInfo> &TempImageInfos);
    void bindPipelineCommand(RHI::CommandListIR &ir, RHI::BindPipelineCommand *command);
    void resolveCommand(RHI::CommandListIR &ir, RHI::RHICommand *command);
    void resolveBarrierCommand(RHI::CommandListIR &ir, RHI::RHICommand *command);

    // commandList Helpers
    void beginRendering(RHI::CommandListIR &);
    void endRendering(RHI::CommandListIR &);
};
} // namespace VKRenderer
