#pragma once

#include <string>
#include <utility>

#include "application/SubApplication.hpp"
#include "config.hpp"
#include "defines.hpp"

#include "renderer/vulkan/VulkanRenderer.hpp"

struct FrameRateData {
    using dur = std::chrono::duration<double, std::milli>;

    dur last = dur::zero();         // milliseconds for the last frame
    dur acc = dur::zero();          // milliseconds accumulator over the last update interval
    float msMean = 0.0f;            // mean milliseconds for the last update interval
    int frameCount = 0;             // frame count for the last update interval
    float updateIntervalSec = 0.5f; // update interval in seconds

    void update(float dtSeconds) {
        if (dtSeconds <= 0.0f) { return; }

        last = std::chrono::duration<double>(dtSeconds);
        acc += std::chrono::duration<double, std::milli>(dtSeconds * 1000.0);
        frameCount++;

        if (acc >= std::chrono::duration<double>(updateIntervalSec)) {
            msMean = acc.count() / double(frameCount);
            acc = dur::zero();
            frameCount = 0;
        }
    }

    void reset() {
        *this = {};
        updateIntervalSec = 0.5f;
    }
};

class Application {

public: // functions
    Application();
    Application(const ApplicationConfig &config);
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    ~Application();

    void run();

    void stop();

    void setConfig(const ApplicationConfig &config);

    void setTitle(std::string title);

    const std::string &getTitle() const { return m_config.title; }

    void setFpsUpdateInterval(float updateInterval) { m_config.fpsUpdateInterval = updateInterval; }

    const FrameRateData &getFrameRateData() const { return m_frameRateData; }

    void renderFrame();

    bool isRunning() const { return m_running; }

    Vec2i getWindowSize() const { return {m_config.width, m_config.height}; }

    bool isAnimating() const { return m_config.animation; }

    void setAnimating(bool animating) { m_config.animation = animating; }

    bool isVsync() const { return m_config.vsync == 1; }

    void setVsync(int vsync) { m_config.vsync = vsync; }

    void setSubApp(const std::string &name);
    void setSubApp(std::unique_ptr<SubApplication> subapp);

protected:
    void init();
    void shutdown();
    void handleEvent();
    void displayUI();
    void resize(const Vec2i &size);
    void menuBarUI();
    void subAppSelectionUI();
    void toggleFullscreen();
    void applyUIScale();

private:
    void destroyActiveSubApp();
    void activateSubApp(std::unique_ptr<SubApplication> next, const std::string &forcedName);

    template <typename Method, typename... Args>
    void callSubApp(Method m, const char *phase, Args &&...args);

    std::unique_ptr<SubApplication> m_activeSubApp;
    std::string m_activeSubAppName;

private: // member variables
    FrameRateData m_frameRateData;
    GLFWwindow *m_window = nullptr;

    ApplicationConfig m_config;

    bool m_running = false;
    int m_lastSyncType = -1;
    int m_targetFPS = 120;

    VKRenderer::Renderer m_renderer;
};