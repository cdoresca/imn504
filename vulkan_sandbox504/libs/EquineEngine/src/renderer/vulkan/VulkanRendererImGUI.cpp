#include "VulkanRenderer.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace VKRenderer {
void Renderer::initImgui() {
    imguiCreateDescriptorPool();
    ImGui::CreateContext();
    // Setup controls.
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_instance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = m_logicalDevice;
    initInfo.QueueFamily = m_queueFamilyIndices.graphicsQueueIndex;
    initInfo.Queue = m_graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_imguiDescriptorPool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = EE::Config::kMaxFramesInFlight;
    initInfo.ImageCount = EE::Config::kMaxFramesInFlight;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.UseDynamicRendering = true;
    initInfo.ColorAttachmentFormat = (VkFormat)m_frameData[0].swapchainImage.format;

    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
        return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
    }, &m_instance);
    ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);
}

void Renderer::cleanupImgui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_logicalDevice.destroyDescriptorPool(m_imguiDescriptorPool);
}

void Renderer::imguiCreateDescriptorPool() {
    vk::DescriptorPoolSize pool_sizes[] = {
        {vk::DescriptorType::eSampler,              1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage,         1000},
        {vk::DescriptorType::eStorageImage,         1000},
        {vk::DescriptorType::eUniformTexelBuffer,   1000},
        {vk::DescriptorType::eStorageTexelBuffer,   1000},
        {vk::DescriptorType::eUniformBuffer,        1000},
        {vk::DescriptorType::eStorageBuffer,        1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment,      1000}
    };
    vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000 * IM_ARRAYSIZE(pool_sizes), IM_ARRAYSIZE(pool_sizes), pool_sizes);
    m_imguiDescriptorPool = m_logicalDevice.createDescriptorPool(pool_info);
}

void Renderer::renderUI(uint imageIndex) {
    m_frameData[m_currentFrame].uiCommandBuffer.begin(vk::CommandBufferBeginInfo());
    vk::RenderingAttachmentInfo colorAttachmentInfo{m_frameData[imageIndex].swapchainImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, {}, {}, {}, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, vk::ClearValue({.0f, .0f, .0f, .0f})};
    vk::RenderingInfo renderingInfo{{}, vk::Rect2D({0, 0}, {static_cast<uint>(m_windowSize.x), static_cast<uint>(m_windowSize.y)}), 1, {}, 1, &colorAttachmentInfo};
    m_frameData[m_currentFrame].uiCommandBuffer.beginRendering(&renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_frameData[m_currentFrame].uiCommandBuffer);
    m_frameData[m_currentFrame].uiCommandBuffer.endRendering();
    m_frameData[m_currentFrame].uiCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, {vk::ImageMemoryBarrier({}, vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, m_frameData[imageIndex].swapchainImage.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))});
    m_frameData[m_currentFrame].uiCommandBuffer.end();
}

void Renderer::ImGuiNewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
} // namespace VKRenderer