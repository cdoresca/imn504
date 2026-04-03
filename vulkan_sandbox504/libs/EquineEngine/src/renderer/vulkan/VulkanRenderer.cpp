
#include "VulkanRenderer.hpp"

#include <imgui_impl_vulkan.h>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "GLFW/glfw3.h"

#include "UUID.hpp"
#include "VkUtils.hpp"
#include "callback.hpp"
#include "rhi/CommandList.hpp"
#include "rhi/Instrumentation.hpp"
#include "rhi/PipelineDesc.hpp"
#include <set>

/*
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Reduces Windows header bloat
#define NOMINMAX            //  min/max macro conflicts
#include <windows.h>
#endif
*/

namespace VKRenderer {
void Renderer::init(GLFWwindow *window, bool vSync) {
    m_window = window;
    m_vsync = vSync;
    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    createInstance();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    setupDebugMessenger(m_instance, m_debugMessenger);
    createSurface();
    selectPhysicalDevice();
    createDevice();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_logicalDevice);
    createSwapChain();
    createMainSyncObjects();
    createTransientCommandPool();
    createMainCommandBuffers();
    createDescriptorPool();
    createQueryPools();
    initImgui();
    createDefaultRessources();
}

void Renderer::cleanup() {
    cleanupSwapChain();

    // cleanup imgui
    cleanupImgui();

    // cleanup command pools
    m_logicalDevice.destroyCommandPool(m_transientPool);
    for (FrameData &framedata : m_frameData) {
        m_logicalDevice.destroyCommandPool(framedata.commandPool);
    }

    // cleanup semaphores & fences
    for (MainSyncObject &syncObject : m_mainSyncObjects) {
        m_logicalDevice.destroySemaphore(syncObject.presentComplete);
        m_logicalDevice.destroySemaphore(syncObject.renderFinishedSemaphore);
        m_logicalDevice.destroyFence(syncObject.stillInFlightFence);
    }

    // cleanup buffers and textures
    for (auto [uuid, buffer] : m_bufferRegistry) {
        m_logicalDevice.destroyBuffer(buffer.buffer);
        m_logicalDevice.freeMemory(buffer.memory);
    }

    for (auto [uuid, texture] : m_textureRegistry) {
        m_logicalDevice.destroyImage(texture.image);
        m_logicalDevice.freeMemory(texture.memory);
        m_logicalDevice.destroyImageView(texture.imageView);
        if (texture.hasImGui) {
            ImGui_ImplVulkan_RemoveTexture(texture.ImGuiDescriptorSet);
        }
    }

    for (auto [uuid, sampler] : m_samplerRegistry) {
        m_logicalDevice.destroySampler(sampler);
    }

    // cleanup UBOs
    for (FrameData &frameData : m_frameData) {
        for (int i = 0; i < frameData.pipelineIDs.size(); i++) {
            for (auto [name, ubo] : frameData.ubos[i]) {
                m_logicalDevice.unmapMemory(ubo.memory);
                m_logicalDevice.destroyBuffer(ubo.buffer);
                m_logicalDevice.freeMemory(ubo.memory);
            }
        }
        m_logicalDevice.destroyQueryPool(frameData.perfCounters.pipelineStatisticsQueryPool);
        m_logicalDevice.destroyQueryPool(frameData.perfCounters.timestampQueryPool);
        m_logicalDevice.destroyQueryPool(frameData.perfCounters.meshPrimitivesQueryPool);
    }
    // cleanup pipelines & render passes
    for (auto [name, pipeline] : m_pipelineRegistry) {
        m_logicalDevice.destroyPipeline(pipeline.pipeline);
        m_logicalDevice.destroyPipelineLayout(pipeline.layout);
        for (auto layout : pipeline.setLayouts) {
            m_logicalDevice.destroyDescriptorSetLayout(layout);
        }
    }

    m_logicalDevice.destroyDescriptorPool(m_descriptorPool);
    m_logicalDevice.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    if (EE::Config::kEnableValidationLayers) { m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger); }
    m_instance.destroy();
}

void Renderer::setVSync(int syncType) {
    if (syncType == m_vsync) {
        return;
    }
    m_logicalDevice.waitIdle();
    m_vsync = syncType;
    recreateSwapchain();
    // Todo: seems to lock when we minimize then de-minimize the window, need to investigate
}

void Renderer::createInstance() {
    if (EE::Config::kEnableValidationLayers && !checkValidationLayerSupport()) { throw std::runtime_error("Validation layers requested, but not available!"); }
    std::cout << "Validation layers are" << (EE::Config::kEnableValidationLayers && !checkValidationLayerSupport() ? " " : " not ") << "enabled." << std::endl;

    vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "EquineEngine", VK_MAKE_VERSION(0, 0, 1), VK_API_VERSION_1_3);
    vk::InstanceCreateInfo instanceInfo({}, &appInfo);
    // interface with the window system
    auto extensions = getRequiredInstanceExtensions();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    std::vector<const char *> instanceLayers = {};
    // Dirty Reshade integration

    // const char *vkLayerPath = "libs/reshade/setup/";
    // Win::SetEnvironmentVariableA("VK_ADD_LAYER_PATH", vkLayerPath);
    // instanceLayers.push_back("VK_LAYER_reshade");
    /*
const char* vkLayerPath = "lib/";
Win::SetEnvironmentVariableA("VK_ADD_LAYER_PATH", vkLayerPath);
instanceLayers.push_back("VK_LAYER_reshade");*/

    // validation layers
    vk::DebugUtilsMessengerCreateInfoEXT instanceDebugMessengerInfo{};
    populateDebugMessengerCreateInfo(instanceDebugMessengerInfo);
    if (EE::Config::kEnableValidationLayers) {
        instanceLayers.push_back(EE::Config::kValidationLayerName);
        instanceInfo.pNext = &instanceDebugMessengerInfo;
    }
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    instanceInfo.ppEnabledLayerNames = instanceLayers.data();

    // instanceInfo.pNext = nullptr;
    listAvailableExtensions();

    // create the instance
    m_instance = vk::createInstance(instanceInfo);
}

// TODO better WSI
void Renderer::createSurface() {
    // TODO: integrate error handler with GLFW
    VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, reinterpret_cast<VkSurfaceKHR *>(&m_surface));
    if (result != VK_SUCCESS) { throw std::runtime_error("Failed to create a Vulkan surface to the GLFW window."); }
}

void Renderer::selectPhysicalDevice() {
    auto devices = m_instance.enumeratePhysicalDevices();
    if (devices.size() == 0) { throw std::runtime_error("Failed to find GPUs with Vulkan support."); }
    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }
    if (!m_physicalDevice) { throw std::runtime_error("Failed to find a suitable GPU."); }
}

void Renderer::createDevice() {
    std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;
    float queuePriority = 1.0f;
    std::set<int> uniqueQueueFamilies = {m_queueFamilyIndices.graphicsQueueIndex, m_queueFamilyIndices.computeQueueIndex, m_queueFamilyIndices.presentQueueIndex};
    for (int queueFamilyIndex : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
        queuesCreateInfo.push_back(queueCreateInfo);
    }
    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
    physicalDeviceFeatures2.features.setPipelineStatisticsQuery(true);
    physicalDeviceFeatures2.features.setGeometryShader(true);
    physicalDeviceFeatures2.features.setWideLines(true);
    physicalDeviceFeatures2.features.setDrawIndirectFirstInstance(true);
    physicalDeviceFeatures2.features.setMultiDrawIndirect(true);
    vk::PhysicalDeviceVulkan11Features physicalDeviceVulkan11Features;
    physicalDeviceVulkan11Features.setShaderDrawParameters(true);
    vk::PhysicalDeviceVulkan12Features physicalDeviceVulkan12Features;
    physicalDeviceVulkan12Features.setHostQueryReset(true);
    physicalDeviceVulkan12Features.setScalarBlockLayout(true);
    physicalDeviceVulkan12Features.setStorageBuffer8BitAccess(true);
    vk::PhysicalDeviceVulkan13Features physicalDeviceVulkan13Features;
    physicalDeviceVulkan13Features.setSynchronization2(true);
    physicalDeviceVulkan13Features.setDynamicRendering(true);
    // Merge core + optional (application-provided) device extensions.
    std::vector<const char *> mergedExtensions = EE::Config::mergeExtensions(m_optionalDeviceExtensions);

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features> deviceParams = {
        {{}, static_cast<uint32_t>(queuesCreateInfo.size()), queuesCreateInfo.data(), 0, nullptr, static_cast<uint32_t>(mergedExtensions.size()), mergedExtensions.data()},
        physicalDeviceFeatures2,
        physicalDeviceVulkan11Features,
        physicalDeviceVulkan12Features,
        physicalDeviceVulkan13Features
    };
    m_logicalDevice = m_physicalDevice.createDevice(deviceParams.get<vk::DeviceCreateInfo>());
    m_deviceLimits = m_physicalDevice.getProperties().limits;
    if (m_deviceLimits.timestampPeriod == 0) {
        throw std::runtime_error{"The selected device does not support timestamp queries!"};
    }

    m_graphicsQueue = m_logicalDevice.getQueue(m_queueFamilyIndices.graphicsQueueIndex, 0);
    m_presentQueue = m_logicalDevice.getQueue(m_queueFamilyIndices.presentQueueIndex, 0);
}

void Renderer::createMainSyncObjects() {
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
    for (size_t i = 0; i < EE::Config::kMaxFramesInFlight; i++) {
        // m_MainSyncObjects[i] = {};
        m_mainSyncObjects[i].presentComplete = m_logicalDevice.createSemaphore(semaphoreCreateInfo);
        m_logicalDevice.setDebugUtilsObjectNameEXT({vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)m_mainSyncObjects[i].presentComplete, "presentCompleteSem"}, VULKAN_HPP_DEFAULT_DISPATCHER);
        m_mainSyncObjects[i].renderFinishedSemaphore = m_logicalDevice.createSemaphore(semaphoreCreateInfo);
        m_logicalDevice.setDebugUtilsObjectNameEXT({vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)m_mainSyncObjects[i].renderFinishedSemaphore, "renderFinishedSem"}, VULKAN_HPP_DEFAULT_DISPATCHER);
        m_mainSyncObjects[i].stillInFlightFence = m_logicalDevice.createFence(fenceCreateInfo);
        m_logicalDevice.setDebugUtilsObjectNameEXT({vk::ObjectType::eFence, (uint64_t)(VkFence)m_mainSyncObjects[i].stillInFlightFence, &"stillInFlightFence"[i]}, VULKAN_HPP_DEFAULT_DISPATCHER);
    }
}

void Renderer::render(bool withUI) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    if (width == 0 || height == 0) {
        throw std::runtime_error("Window was minimized without disabling rendering first.");
        return; // window is minimized
    }
    vk::Result result = m_logicalDevice.waitForFences(1, &m_mainSyncObjects[m_currentFrame].stillInFlightFence, VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess) { throw std::runtime_error("Failed to wait for a fence."); }

    result = m_logicalDevice.acquireNextImageKHR(m_swapChain, UINT64_MAX, m_mainSyncObjects[m_currentFrame].presentComplete, VK_NULL_HANDLE, &m_acquireImageIndex);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
    // SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until presen() in case number of swapchain images will change on resize

    result = m_logicalDevice.resetFences(1, &m_mainSyncObjects[m_currentFrame].stillInFlightFence);
    if (result != vk::Result::eSuccess) { throw std::runtime_error("Failed to reset in flight fence."); }
    updateUBOs(m_currentFrame);
    updatePushConstants(m_currentFrame);
    makeQueryResultsAvailable(m_currentFrame);
    resetQueryPools(m_currentFrame);

    m_frameData[m_currentFrame].commandBuffer.reset({});
    vk::Viewport viewport{0, 0, static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y), 0.0f, 1.0f};
    vk::Rect2D scissor{
        {0,                                 0                                },
        {static_cast<uint>(m_windowSize.x), static_cast<uint>(m_windowSize.y)}
    };

    std::vector<vk::CommandBuffer> submitCommandBuffers;

    m_frameData[m_currentFrame].commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    m_frameData[m_currentFrame].commandBuffer.setViewport(0, 1, &viewport);
    m_frameData[m_currentFrame].commandBuffer.setScissor(0, 1, &scissor);
    m_frameData[m_currentFrame].commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {}, {}, {},
        {vk::ImageMemoryBarrier({}, vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_frameData[m_currentFrame].swapchainImage.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))}
    );
    m_frameData[m_currentFrame].commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        {}, {}, {},
        {vk::ImageMemoryBarrier({}, vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_frameData[m_currentFrame].depth.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))}
    );

    if (!m_intermediateCommandList.compileCommands(&m_frameData[m_currentFrame], false)) {
        // Dynamic rendering handles the clear natively
        vk::RenderingAttachmentInfo colorAttachmentInfoMain{m_frameData[m_currentFrame].swapchainImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, {}, {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::ClearValue(std::array<float,4>({0.0f, 0.0f, 0.0f, 1.0f}))};
        vk::RenderingAttachmentInfo depthAttachmentInfo{m_frameData[m_currentFrame].depth.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, {}, {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::ClearValue({1.f, 0})};

        vk::RenderingInfo renderingInfoMain{{}, vk::Rect2D({0, 0}, {static_cast<uint>(m_frameData[m_currentFrame].swapchainImage.size.x), static_cast<uint>(m_frameData[m_currentFrame].swapchainImage.size.y)}), 1, {}, 1, &colorAttachmentInfoMain, &depthAttachmentInfo};

        m_frameData[m_currentFrame].commandBuffer.beginRendering(&renderingInfoMain);
        m_frameData[m_currentFrame].commandBuffer.endRendering();
    }

    m_frameData[m_currentFrame].commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
        {}, {}, {},
        {vk::ImageMemoryBarrier(vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_frameData[m_currentFrame].depth.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))}
    );

    if (!withUI) {
        m_frameData[m_currentFrame].commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
            {}, {}, {},
            {vk::ImageMemoryBarrier({}, vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_frameData[m_currentFrame].swapchainImage.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))}
        );
    }
    m_frameData[m_currentFrame].commandBuffer.end();

    submitCommandBuffers.push_back(m_frameData[m_currentFrame].commandBuffer);
    if (withUI) {
        renderUI(m_currentFrame); // Passes currentFrame since UI uses m_frameData[m_currentFrame] too!
        submitCommandBuffers.push_back(m_frameData[m_currentFrame].uiCommandBuffer);
    }
    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo(1, &m_mainSyncObjects[m_currentFrame].presentComplete, &waitStages, submitCommandBuffers.size(), submitCommandBuffers.data(), 1, &m_mainSyncObjects[m_currentFrame].renderFinishedSemaphore);

    result = m_graphicsQueue.submit(1, &submitInfo, m_mainSyncObjects[m_currentFrame].stillInFlightFence);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to submit draw command buffer.");
}

void Renderer::present() {
    vk::PresentInfoKHR presentInfo(1, &m_mainSyncObjects[m_currentFrame].renderFinishedSemaphore, 1, &m_swapChain, &m_acquireImageIndex);
    vk::Result result = m_graphicsQueue.presentKHR(&presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        recreateSwapchain();
        if (result == vk::Result::eErrorOutOfDateKHR) {
            return;
        }
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swapchain image.");
    }
    m_currentFrame = (m_currentFrame + 1) % m_frameData.size();
}

void Renderer::waitIdle() {
    m_logicalDevice.waitIdle();
}

void Renderer::updateUBOs(uint imageIndex) {
    // TODO: only update the UBOs that have changed
    // TODO: error if no UBO was bound
    uint count = 0;
    for (auto &pipelineId : m_frameData[imageIndex].pipelineIDs) {
        auto pipeline = m_pipelineRegistry[pipelineId].pipelineData;
        auto ubos = pipeline->getUbos();
        for (auto variants : ubos) {
            auto uboData = variants[0].buildBufferForUpload();
            memcpy(m_frameData[imageIndex].ubos[count][variants[0].layout.uboName].gpuPtr, uboData.data(), uboData.size());
        }
        count++;
    }
}

void Renderer::updatePushConstants(uint imageIndex) {
    for (auto &pipelineId : m_frameData[imageIndex].pipelineIDs) {
        Pipeline &pipeline = m_pipelineRegistry[pipelineId];
        if (pipeline.pipelineData->getPushConstants() != nullptr) {
            // copy to push constant to pipeline.pushConstants
            auto buf = pipeline.pipelineData->getPushConstants()->buildBufferForUpload();
            std::memcpy(pipeline.pushConstantsData.data(), buf.data(), buf.size());
        }
    }
}

void Renderer::resetQueryPools(uint imageIndex) {
    m_logicalDevice.resetQueryPool(m_frameData[imageIndex].perfCounters.pipelineStatisticsQueryPool, 0, 1000);
    m_logicalDevice.resetQueryPool(m_frameData[imageIndex].perfCounters.timestampQueryPool, 0, 1000);
    // m_logicalDevice.resetQueryPool(m_frameData[imageIndex].perfCounters.meshPrimitivesQueryPool, 0, 1000);
}

void Renderer::makeQueryResultsAvailable(uint imageIndex) {
    if (m_perfCounterGlobals.pipelineStatisticsQueryIndex > 0) {
        if (m_perfCounterGlobals.pipelineStatisticsResults.size() < m_perfCounterGlobals.pipelineStatisticsQueryIndex) {
            m_perfCounterGlobals.pipelineStatisticsResults.resize(m_perfCounterGlobals.pipelineStatisticsQueryIndex);
        }
        auto pipelineStatsResult = m_logicalDevice.getQueryPoolResults(m_frameData[imageIndex].perfCounters.pipelineStatisticsQueryPool, 0, m_perfCounterGlobals.pipelineStatisticsQueryIndex, m_perfCounterGlobals.pipelineStatisticsResults.size() * 3 * sizeof(uint), m_perfCounterGlobals.pipelineStatisticsResults.data(), sizeof(uint) * 3, {});
        if (pipelineStatsResult != vk::Result::eSuccess && pipelineStatsResult != vk::Result::eNotReady) {
            throw std::runtime_error("Failed to get pipeline statistics query pool results.");
        }
    }
    if (m_perfCounterGlobals.timestampQueryIndex > 0) {
        if (m_perfCounterGlobals.timestampResults.size() < m_perfCounterGlobals.timestampQueryIndex) {
            m_perfCounterGlobals.timestampResults.resize(m_perfCounterGlobals.timestampQueryIndex);
        }
        {
            auto _ = m_logicalDevice.getQueryPoolResults(m_frameData[imageIndex].perfCounters.timestampQueryPool, 0, m_perfCounterGlobals.timestampQueryIndex, m_perfCounterGlobals.timestampResults.size() * sizeof(uint64_t), m_perfCounterGlobals.timestampResults.data(), sizeof(uint64_t), vk::QueryResultFlagBits::e64);
            if (m_perfCounterGlobals.timestampResults.size() < m_perfCounterGlobals.timestampQueryIndex) {
                throw std::runtime_error("Failed to get timestamp query pool results.");
            }
        }
    }
    if (m_perfCounterGlobals.meshPrimitivesQueryIndex > 0) {
        if (m_perfCounterGlobals.meshPrimitivesResults.size() < m_perfCounterGlobals.meshPrimitivesQueryIndex) {
            m_perfCounterGlobals.meshPrimitivesResults.resize(m_perfCounterGlobals.meshPrimitivesQueryIndex);
        }
        auto meshPrimitivesResult = m_logicalDevice.getQueryPoolResults(m_frameData[imageIndex].perfCounters.meshPrimitivesQueryPool, 0, m_perfCounterGlobals.meshPrimitivesQueryIndex, m_perfCounterGlobals.meshPrimitivesResults.size() * sizeof(uint), m_perfCounterGlobals.meshPrimitivesResults.data(), sizeof(uint), {});
        if (meshPrimitivesResult != vk::Result::eSuccess && meshPrimitivesResult != vk::Result::eNotReady) {
            throw std::runtime_error("Failed to get mesh primitives query pool results.");
        }
    }
}

RHI::PipelineStatistics Renderer::getPipelineStatistics(const RHI::PipelineDesc &pipelineDesc) {
    RHI::PipelineStatistics stats;
    if (!pipelineDesc.hasTiming && !pipelineDesc.hasFineInstrumentation) {
        return stats;
    }
    if (m_perfCounterGlobals.pipelineStatisticsResults.size() + m_perfCounterGlobals.timestampResults.size() + m_perfCounterGlobals.meshPrimitivesResults.size() == 0) {
        return stats;
    }
    if (m_pipelineRegistry.find(pipelineDesc.uuid) == m_pipelineRegistry.end()) {
        return stats;
    }

    Pipeline &pipeline = m_pipelineRegistry[pipelineDesc.uuid];
    uint size = 0;
    if (pipelineDesc.hasTiming) {
        size = pipeline.instrumentationInfo.timerIDs.size();
    } else {
        size = pipeline.instrumentationInfo.pipelineStatsIDs.size();
    }
    stats.callStatistics.resize(size);
    for (int drawcallIndex = 0; drawcallIndex < stats.callStatistics.size(); drawcallIndex++) {
        if (pipelineDesc.hasFineInstrumentation) {
            stats.callStatistics[drawcallIndex].meshPrimitiveCount = m_perfCounterGlobals.meshPrimitivesResults[pipeline.instrumentationInfo.meshPrimIDs[drawcallIndex]];
            stats.callStatistics[drawcallIndex].computeInvocationCount = m_perfCounterGlobals.pipelineStatisticsResults[pipeline.instrumentationInfo.pipelineStatsIDs[drawcallIndex]][0];
            stats.callStatistics[drawcallIndex].taskInvocationCount = m_perfCounterGlobals.pipelineStatisticsResults[pipeline.instrumentationInfo.pipelineStatsIDs[drawcallIndex]][1];
            stats.callStatistics[drawcallIndex].meshInvocationCount = m_perfCounterGlobals.pipelineStatisticsResults[pipeline.instrumentationInfo.pipelineStatsIDs[drawcallIndex]][2];
        }
        if (pipelineDesc.hasTiming) {
            stats.callStatistics[drawcallIndex].gpuTime = nanosecondsF((m_perfCounterGlobals.timestampResults[pipeline.instrumentationInfo.timerIDs[drawcallIndex] + 1] - m_perfCounterGlobals.timestampResults[pipeline.instrumentationInfo.timerIDs[drawcallIndex]]) * (double)m_deviceLimits.timestampPeriod);
            stats.gpuTime += stats.callStatistics[drawcallIndex].gpuTime;
        }
    }

    return stats;
}

void Renderer::createDefaultRessources() {
    // create default sampler
    gDefaultLinearSampler.generate();
    vk::SamplerCreateInfo samplerCreateInfo({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.minLod = -1000;
    samplerCreateInfo.maxLod = 1000;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    m_samplerRegistry[gDefaultLinearSampler] = m_logicalDevice.createSampler(samplerCreateInfo);
}

RHI::TextureDesc Renderer::getCurrentDepthTextureDesc() {
    RHI::TextureDesc desc(RHI::TextureDim::e2D, m_frameData[m_acquireImageIndex].depth.size.x, m_frameData[m_acquireImageIndex].depth.size.y, 1, 1, 1, RHI::Format::eNone, RHI::ShaderAccessMode::eReadOnly, "DefaultDepth");
    // Reuse the UUID of the internal depth image, so descriptor writes reference the same VkImageView
    desc.uuid = m_frameData[m_acquireImageIndex].depth.uuidOverride;
    return desc;
}

RHI::SamplerDesc Renderer::getDefaultLinearSamplerDesc() {
    RHI::SamplerDesc s(RHI::FilterMode::eLinear, RHI::FilterMode::eLinear, RHI::MipMapMode::eLinear, RHI::AddressMode::eRepeat, RHI::AddressMode::eRepeat, RHI::AddressMode::eRepeat, 0.f, 1);
    s.uuid = gDefaultLinearSampler;
    return s;
}
} // namespace VKRenderer