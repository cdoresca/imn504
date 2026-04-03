#pragma once
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>


namespace VKRenderer {
struct QueueFamilyIndices;
struct SwapChainDetails;
} // namespace VKRenderer

std::vector<const char *> getRequiredInstanceExtensions();

VKRenderer::QueueFamilyIndices queryQueueFamilies(vk::PhysicalDevice &device, vk::SurfaceKHR &surface);

VKRenderer::SwapChainDetails querySwapChainDetails(vk::PhysicalDevice &device, vk::SurfaceKHR &surface);

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes, int syncType);

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities, GLFWwindow *window);

vk::WriteDescriptorSet createDescriptorWrite(vk::DescriptorSet p_dstSet, uint32_t p_binding, vk::DescriptorType p_type, vk::DescriptorBufferInfo *p_bufferInfo, uint32_t p_descriptorCount);

vk::WriteDescriptorSet createDescriptorWrite(vk::DescriptorSet p_dstSet, uint32_t p_binding, vk::DescriptorType p_type, vk::DescriptorImageInfo *p_imageInfo, uint32_t p_descriptorCount);

bool checkValidationLayerSupport();

bool checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char *> &requiredDeviceExtensions);

void listAvailableExtensions();