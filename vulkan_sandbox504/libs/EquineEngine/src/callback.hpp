#pragma once
#include "EngineConfig.hpp"
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    std::string message(pCallbackData->pMessage);

    if (message.find("ReShade") != std::string::npos ||
        message.find("obs-studio-hook") != std::string::npos ||
        message.find("GalaxyOverlayVkLayer") != std::string::npos) {
        std::cout << "clutter: " << message << std::endl;
    } else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            std::cout << "debug: " << message << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cout << "info: " << message << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            std::cout << "warning: " << message << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            std::cout << "error: " << message << std::endl;
            throw std::runtime_error("Validation error.");
            break;
        default:
            std::cout << "unknown: " << message << std::endl;
            break;
        }
    } else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        std::cout << "general: " << message << std::endl;
    } else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        std::cout << "performance: " << message << std::endl;
    } else {
        std::cout << "unknown: " << message << std::endl;
    }

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT &createInfoDebugMessenger) {
    using MessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MessageType = vk::DebugUtilsMessageTypeFlagBitsEXT;

    createInfoDebugMessenger = vk::DebugUtilsMessengerCreateInfoEXT{
        {},
        MessageSeverity::eVerbose | MessageSeverity::eWarning | MessageSeverity::eError,
        MessageType::eGeneral | MessageType::eValidation | MessageType::ePerformance,
        reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(debugCallback),
        nullptr};
}

inline void setupDebugMessenger(vk::Instance p_instance, vk::DebugUtilsMessengerEXT &p_debugMessenger) {
    if (!EE::Config::kEnableValidationLayers) { return; }

    vk::DebugUtilsMessengerCreateInfoEXT createInfoDebugMessenger{};
    populateDebugMessengerCreateInfo(createInfoDebugMessenger);
    p_debugMessenger = p_instance.createDebugUtilsMessengerEXT(createInfoDebugMessenger, nullptr);
}