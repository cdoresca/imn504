// Central engine-only configuration (no application/window concerns)
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <string>

namespace EE {
namespace Config {

constexpr uint8_t kMaxFramesInFlight = 2;
constexpr uint8_t kMaxDescriptorSets = 8; // Most HW supports at least 8-32
constexpr const char *kValidationLayerName = "VK_LAYER_KHRONOS_validation";

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = false;
#endif

// Core (hard) device extensions the engine itself requires to function.
inline const std::vector<const char *> &coreDeviceExtensions() {
    static const std::vector<const char *> exts = {
        "VK_KHR_swapchain"};
    return exts;
}

// App can request additional extensions
inline std::vector<const char *> mergeExtensions(const std::vector<const char *> &optionalExts) {
    std::vector<const char *> merged = coreDeviceExtensions();
    for (auto *ext : optionalExts) {
        if (std::find_if(merged.begin(), merged.end(), [&](const char *e) { return std::string_view(e) == ext; }) == merged.end()) {
            merged.push_back(ext);
        }
    }
    return merged;
}

} // namespace Config
} // namespace EE
