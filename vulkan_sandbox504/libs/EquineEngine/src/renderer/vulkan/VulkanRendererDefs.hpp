#pragma once
#include "EngineConfig.hpp"
#include "UUID.hpp"
#include "defines.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <unordered_map>
#include <vector>
namespace RHI {
class ManagedUBO;
class PipelineDesc;
union CommandParameter;
} // namespace RHI

namespace VKRenderer {
struct QueueFamilyIndices {
    int32_t graphicsQueueIndex = -1;
    int32_t computeQueueIndex = -1;
    int32_t presentQueueIndex = -1;

    bool isComplete() { return graphicsQueueIndex != -1 && computeQueueIndex != -1 && presentQueueIndex != -1; }
};

struct SwapChainDetails {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities{};
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Texture {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView imageView;
    vk::Format format;
    Vec3u size = {0, 0, 0};
    bool hasImGui = false;
    vk::DescriptorSet ImGuiDescriptorSet;
    // Link to a RHI::ResourceDesc UUID when exposing internal images to RHI
    UUID uuidOverride;
    // True when this Texture refers to an externally owned image (e.g., swapchain/depth),
    // so cleanup should not destroy image/memory/imageView here.
    bool externalOwned = false;
};

struct SampledTexture : public Texture {
    vk::Sampler sampler;
};

struct Buffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

struct UBO : public Buffer {
    void *gpuPtr;
    RHI::ManagedUBO *uboData;
};

struct Instrumentation {
    std::vector<uint> pipelineStatsIDs;
    std::vector<uint> timerIDs;
    std::vector<uint> meshPrimIDs;
};

struct Pipeline {
    RHI::PipelineDesc *pipelineData = nullptr; // non-owning reference, user owns the PipelineDesc
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    std::vector<vk::DescriptorSetLayout> setLayouts;
    uint descriptorBindingsCount = 0;
    std::array<byte, 128> pushConstantsData;
    Instrumentation instrumentationInfo;
};

struct PerfCounterGlobals {
    uint pipelineStatisticsQueryIndex = 0;
    uint timestampQueryIndex = 0;
    uint meshPrimitivesQueryIndex = 0;
    vk::QueryPipelineStatisticFlags pipelineStatsFlags = vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

    std::vector<std::array<uint, 3>> pipelineStatisticsResults; // N is the number of PipelineStatisticsFlags
    std::vector<uint64_t> timestampResults;
    std::vector<uint> meshPrimitivesResults;
    // 0: timestampQueryIndex
    uint generateTimestampCLParams(Pipeline &pipeline, std::vector<RHI::CommandParameter *> &parameters);
    // 0: pipelineStatsQueryIndex, 1: meshPrimitivesQueryIndex
    uint generatePerfCounterCLParamsWMeshShader(Pipeline &pipeline, std::vector<RHI::CommandParameter *> &parameters);
};

struct PerformanceCounters {
    vk::QueryPool pipelineStatisticsQueryPool;
    vk::QueryPool timestampQueryPool;
    vk::QueryPool meshPrimitivesQueryPool;
};

struct MainSyncObject {
    vk::Semaphore presentComplete;         // signaled when the image is ready to be rendered into
    vk::Semaphore renderFinishedSemaphore; // signaled when the rendering is done (for the present queue)
    vk::Fence stillInFlightFence;          // signaled when the submited frame is done rendering (to prevent submitting too many frames)
};

struct FrameData {
    uint frameNumber;
    // constant in the lifetime of the application
    vk::CommandPool commandPool;
    vk::CommandBuffer uiCommandBuffer;
    PerformanceCounters perfCounters;

    // present targets
    Texture swapchainImage;
    Texture depth;

    // reset at rebuild
    vk::CommandBuffer commandBuffer; // temporary need more than one when we will compile a frame graph
    std::vector<std::unordered_map<std::string, UBO>> ubos;
    std::vector<std::array<vk::DescriptorSet, EE::Config::kMaxDescriptorSets>> descriptorSets;
    std::vector<UUID> pipelineIDs;
};

} // namespace VKRenderer
