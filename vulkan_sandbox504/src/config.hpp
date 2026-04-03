#pragma once

// Application-side configuration (window, UI, runtime behavior) only
#include <filesystem>
#include <iostream>
#include <array>
#include <vector>

#define ROOT_IDENTIFIER ".root"

constexpr unsigned int DEFAULT_WIDTH = 1337;
constexpr unsigned int DEFAULT_HEIGHT = 744;

// These are required for this app's current feature set
inline const std::vector<const char*>& appRequiredVulkanExtensions() {
    static const std::vector<const char*> exts = {
        "VK_EXT_mesh_shader",
    };
    return exts;
}

inline void setWorkingDirectoryToProjectRoot() {
    std::filesystem::path exePath = std::filesystem::current_path();
    unsigned int maxDepth = 10;
    while (exePath.has_parent_path() && !std::filesystem::exists(exePath / ROOT_IDENTIFIER) && maxDepth > 0) {
        exePath = exePath.parent_path();
        maxDepth--;
    }

    if (!std::filesystem::exists(exePath / ROOT_IDENTIFIER)) {
        std::cerr << "Project root not found."
                  << "Are you running the executable from the project folder?"
                  << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::filesystem::current_path(exePath);
}

// ================= Runtime Application Config =====================
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>


struct ApplicationConfig {
    std::string title = "Vulkan Sanbox";
    unsigned int width = DEFAULT_WIDTH;
    unsigned int height = DEFAULT_HEIGHT;
    int vsync = 0; // 0 = no sync, 1 = vsync, 2 = target sync
    bool animation = true;
    float fpsUpdateInterval = 0.5f;
    float uiScale = 1.0f;    // manual UI scale
    bool uiScaleAuto = true; // auto derive from OS scale
    std::string startSubAppName;
};

inline ApplicationConfig loadApplicationConfigFromFile(const std::string &path, bool verbose = true) {
    ApplicationConfig cfg;
    std::ifstream f(path);
    if (!f) {
        if (verbose) std::cerr << "[Config] File '" << path << "' not found. Using defaults.\n";
        return cfg;
    }

    std::string content((std::istreambuf_iterator<char>(f)), {});
    auto trim = [](std::string s) {
        auto b = s.find_first_not_of(" \t\n\r");
        auto e = s.find_last_not_of(" \t\n\r");
        return (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
    };
    auto unquote = [](std::string s) {
        return (s.size() >= 2 && s.front() == '"' && s.back() == '"') ? s.substr(1, s.size() - 2) : s;
    };
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    };

    // strip { }
    auto lb = content.find('{'), rb = content.rfind('}');
    if (lb != std::string::npos && rb != std::string::npos) content = content.substr(lb + 1, rb - lb - 1);

    // key → handler map (flat JSON)
    using Setter = std::function<void(const std::string &)>;
    std::unordered_map<std::string, Setter> setters{
        {"title",             [&](auto v) { cfg.title = unquote(v); }                                               },
        {"width",             [&](auto v) { try{cfg.width = std::max(1, std::stoi(v));}catch(...){} }               },
        {"height",            [&](auto v) { try{cfg.height= std::max(1, std::stoi(v));}catch(...){} }               },
        {"vsync",             [&](auto v) { try{cfg.vsync = std::clamp(std::stoi(v),0,2);}catch(...){} }            },
        {"animation",         [&](auto v) { cfg.animation = (lower(v) == "true" || v == "1"); }                     },
        {"fpsUpdateInterval", [&](auto v) { try{cfg.fpsUpdateInterval = std::max(0.01f,std::stof(v));}catch(...){} }},
        {"uiScale",           [&](auto v) { try{cfg.uiScale=std::clamp(std::stof(v),0.5f,3.0f);}catch(...){} }      },
        {"uiScaleAuto",       [&](auto v) { cfg.uiScaleAuto = (lower(v) == "true" || v == "1"); }                   },
        {"startSubApp",       [&](auto v) { cfg.startSubAppName = unquote(v); }                                     },
    };

    // split by commas
    std::stringstream ss(content);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        auto colon = pair.find(':');
        if (colon == std::string::npos) continue;
        std::string key = unquote(trim(pair.substr(0, colon)));
        std::string val = trim(pair.substr(colon + 1));
        if (auto it = setters.find(key); it != setters.end()) {
            it->second(val);
        } else if (verbose) {
            std::cerr << "[Config] Unknown key: " << key << "\n";
        }
    }

    if (verbose) {
        std::cout << "[Config] Loaded '" << path << "':\n"
                  << "  title=" << cfg.title << "\n"
                  << "  size=" << cfg.width << "x" << cfg.height << "\n"
                  << "  vsync=" << cfg.vsync << "\n"
                  << "  animation=" << (cfg.animation ? "true" : "false") << "\n"
                  << "  fpsUpdateInterval=" << cfg.fpsUpdateInterval << "\n"
                  << "  uiScale=" << cfg.uiScale << " (" << (cfg.uiScaleAuto ? "auto" : "manual") << ")\n";
        if (!cfg.startSubAppName.empty())
            std::cout << "  startSubApp=" << cfg.startSubAppName << "\n";
    }

    return cfg;
}
