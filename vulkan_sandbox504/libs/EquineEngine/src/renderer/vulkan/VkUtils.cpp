#include "VkUtils.hpp"
#include "VulkanRendererDefs.hpp"

#include "EngineConfig.hpp"
#include <iostream>

std::vector<const char *> getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (EE::Config::kEnableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
    // todo: automate this
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return extensions;
}

VKRenderer::QueueFamilyIndices queryQueueFamilies(vk::PhysicalDevice &device, vk::SurfaceKHR &surface) {
    // find a graphics, compute and present queue (can be de same)
    VKRenderer::QueueFamilyIndices famillyIndices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
    uint32_t i = 0;
    while (!famillyIndices.isComplete() && i < queueFamilies.size()) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) { famillyIndices.graphicsQueueIndex = i; }
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) { famillyIndices.computeQueueIndex = i; }
        if (device.getSurfaceSupportKHR(i, surface)) { famillyIndices.presentQueueIndex = i; }
        i++;
    }
    return famillyIndices;
}

VKRenderer::SwapChainDetails querySwapChainDetails(vk::PhysicalDevice &device, vk::SurfaceKHR &surface) {
    VKRenderer::SwapChainDetails details;
    details.surfaceCapabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    // VK_FORMAT_B8G8R8A8_SRGB is the preferred format
    // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR is the preferred color space
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) { return availableFormat; }
    }

    // if preferred format is not available, choose the first available one
    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes, int syncType) {
    // TODO: fix this
    // VK_PRESENT_MODE_MAILBOX_KHR is the preferred mode
    // VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    if (syncType == 1) {
        return vk::PresentModeKHR::eFifo;
    } else if (syncType == 2) {
        return vk::PresentModeKHR::eImmediate;
    } else {
        for (const auto &availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) { return availablePresentMode; }
        }
    }

    // if preferred mode is not available, choose the first available one
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities, GLFWwindow *window) {
    // if current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return surfaceCapabilities.currentExtent; }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    VkExtent2D minImageExtent = surfaceCapabilities.minImageExtent;
    VkExtent2D maxImageExtent = surfaceCapabilities.maxImageExtent;
    actualExtent.width = glm::clamp(actualExtent.width, minImageExtent.width, maxImageExtent.width);
    actualExtent.height = glm::clamp(actualExtent.height, minImageExtent.height, maxImageExtent.height);
    return actualExtent;
}

vk::WriteDescriptorSet createDescriptorWrite(vk::DescriptorSet p_dstSet, uint32_t p_binding, vk::DescriptorType p_type, vk::DescriptorBufferInfo *p_bufferInfo, uint32_t p_descriptorCount) { return vk::WriteDescriptorSet(p_dstSet, p_binding, 0, p_descriptorCount, p_type, nullptr, p_bufferInfo, nullptr); }

vk::WriteDescriptorSet createDescriptorWrite(vk::DescriptorSet p_dstSet, uint32_t p_binding, vk::DescriptorType p_type, vk::DescriptorImageInfo *p_imageInfo, uint32_t p_descriptorCount) { return vk::WriteDescriptorSet(p_dstSet, p_binding, 0, p_descriptorCount, p_type, p_imageInfo, nullptr, nullptr); }

bool checkValidationLayerSupport() {
    std::vector<vk::LayerProperties> layers = vk::enumerateInstanceLayerProperties();
    for (vk::LayerProperties layer : layers) {
        if (strcmp(EE::Config::kValidationLayerName, layer.layerName) == 0) { return true; }
    }
    return false;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char *> &requiredDeviceExtensions) {
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
    for (const char *requiredExtension : requiredDeviceExtensions) {
        bool found = false;
        for (const auto &extension : availableExtensions) {
            if (strcmp(requiredExtension, extension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) { return false; }
    }
    return true;
}

void listAvailableExtensions() {
    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Available extensions:" << std::endl;
    for (const auto &extension : extensions) {
        std::cout << "\t" << extension.extensionName << '\n';
    }
    std::cout << std::endl;
}