#include "application/Application.hpp"

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <thread>

#include "GLFW/glfw3.h"
#include "defines.hpp"
#include "imgui/imgui.h"

#include "application/SubApplication.hpp"
// =========== Utils =============
static void ImGuiFullscreenClear(const ImVec4 &color) {
    // Draw a fullscreen filled rect in background draw list
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    const ImVec2 p0(0.0f, 0.0f);
    const ImVec2 p1 = ImGui::GetIO().DisplaySize;
    bg->AddRectFilled(p0, p1, ImGui::GetColorU32(color));
}

template <typename Method, typename... Args>
void Application::callSubApp(Method m, const char *phase, Args &&...args) {
    if (!m_activeSubApp) return;
    try {
        (m_activeSubApp.get()->*m)(std::forward<Args>(args)...);
    } catch (const std::exception &e) {
        std::cerr << "[SubApp:" << m_activeSubAppName << "] "
                  << phase << " failed: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "[SubApp:" << m_activeSubAppName << "] " << phase << " failed: unknown error\n";
    }
}

// =========== Application Methods Implementation ===========

Application::Application() : Application(ApplicationConfig()) {}

Application::Application(const ApplicationConfig &config)
    : m_config(config) {
    setConfig(config);
    init();
}

void Application::init() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no OpenGL context
    m_window = glfwCreateWindow(m_config.width, m_config.height, m_config.title.c_str(), nullptr, nullptr);
    if (!m_window) throw std::runtime_error("glfwCreateWindow failed");
    m_renderer.setOptionalDeviceExtensions(std::vector<const char *>{});
    m_renderer.init(m_window, m_config.vsync != 0);
    m_renderer.addShaderIncludePaths("shaders");

    // Subapp auto-pick ONLY if config provided a name
    if (!m_config.startSubAppName.empty()) {
        setSubApp(m_config.startSubAppName);
    }

    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetStyle().WindowRounding = 6.0f;
    ImGui::GetStyle().ChildRounding = 6.0f;
    ImGui::GetStyle().FrameRounding = 6.0f;
    ImGui::GetStyle().PopupRounding = 6.0f;
    ImGui::GetStyle().ScrollbarRounding = 6.0f;
    ImGui::GetStyle().GrabRounding = 6.0f;
    ImGui::GetStyle().TabRounding = 6.0f;

    applyUIScale();
}

void Application::destroyActiveSubApp() {
    if (m_activeSubApp) {
        callSubApp(&SubApplication::shutdown, "shutdown");
        m_activeSubApp.reset(); // reset pointer
    }
    m_activeSubAppName.clear();
}

void Application::activateSubApp(std::unique_ptr<SubApplication> next, const std::string &forcedName) {
    if (!next) { return; }

    const std::string subappName = forcedName.empty() ? next->name() : forcedName;

    try {
        next->attachContext(&m_renderer);
        next->init();
        next->resize(m_config.width, m_config.height);
    } catch (const std::exception &e) {
        std::cerr << "[SubApp:" << subappName << "] failed at init: " << e.what() << '\n';
        return;
    }

    destroyActiveSubApp();
    m_activeSubAppName = subappName;
    m_activeSubApp = std::move(next);
}

void Application::setSubApp(const std::string &name) {
    activateSubApp(SubAppRegistry::create(name), name);
}

void Application::setSubApp(std::unique_ptr<SubApplication> subapp) {
    // If null, this is effectively "close current"
    if (!subapp) {
        destroyActiveSubApp();
        return;
    }

    const std::string newName = subapp->name();
    if (newName == m_activeSubAppName) return;

    activateSubApp(std::move(subapp), "");
}

void Application::resize(const Vec2i &size) {
    m_config.width = size.x;
    m_config.height = size.y;
    if (m_activeSubApp) { callSubApp(&SubApplication::resize, "resize", size.x, size.y); }
}

void Application::setTitle(std::string title) {
    m_config.title = std::move(title);
    if (m_window) { glfwSetWindowTitle(m_window, m_config.title.c_str()); }
}

void Application::setConfig(const ApplicationConfig &config) {
    m_config = config;
    setTitle(config.title);
    resize({config.width, config.height});
    setAnimating(config.animation);
    setVsync(config.vsync);
    setFpsUpdateInterval(config.fpsUpdateInterval);
    m_frameRateData.updateIntervalSec = config.fpsUpdateInterval;
}

void Application::handleEvent() {
    if (ImGui::IsKeyPressed(ImGuiKey_Q) && (ImGui::GetIO().KeyCtrl)) {
        std::cout << "CTRL + Q pressed\n";
        stop();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F) && (ImGui::GetIO().KeyCtrl)) {
        std::cout << "CTRL + F pressed\n";
        toggleFullscreen();
    }

    if (m_activeSubApp) { callSubApp(&SubApplication::handleEvent, "handleEvent"); }
}

void Application::stop() {
    std::cout << "Stopping application...\n";
    m_running = false;
}

void Application::run() {
    using clock = std::chrono::steady_clock;

    if (m_running) { throw std::runtime_error("Application already running"); }
    m_running = true;

    auto prev = clock::now();
    auto nextFrame = prev;
    while (m_running) {

        m_running &= glfwWindowShouldClose(m_window) == 0;

        if (!m_config.animation && m_config.vsync != 2) {
            glfwWaitEventsTimeout(0.016); // ~60Hz wakeup
        } else {
            glfwPollEvents();
        }

        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        if (width == 0 || height == 0) { // short sleep to avoid spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (uint(width) != m_config.width || uint(height) != m_config.height) { resize({width, height}); }

        const auto now = clock::now();
        const float dt = std::chrono::duration<float>(now - prev).count();
        prev = now;

        if (m_activeSubApp && m_config.animation) { callSubApp(&SubApplication::animate, "animate", dt); }
        m_frameRateData.update(dt);

        renderFrame();

        if (m_config.vsync == 2) {
            auto frameDur = std::chrono::milliseconds(1000 / std::max(1, m_targetFPS));
            nextFrame += frameDur;
            std::this_thread::sleep_until(nextFrame);
        }
    }
}

void Application::renderFrame() {
    m_renderer.ImGuiNewFrame();
    handleEvent();
    if (m_activeSubApp) {
        callSubApp(&SubApplication::render, "render");
    } else {
        ImGuiFullscreenClear({0.1f, 0.1f, 0.1f, 1.0f});
    }
    displayUI();
    ImGui::Render();
    m_renderer.render();
    m_renderer.present();
}

void Application::displayUI() {
    menuBarUI();

    if (m_activeSubApp) {
        callSubApp(&SubApplication::displayUI, "displayUI");
    } else {
        // Small hint when no subapp picked yet
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("No SubApp", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("No subapp selected. Use the SubApp menu to choose one.");
        ImGui::End();
    }
}

Application::~Application() { shutdown(); }

void Application::shutdown() {
    destroyActiveSubApp();
    m_running = false;
    m_renderer.cleanup();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::menuBarUI() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) { stop(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::RadioButton("No Sync", &m_config.vsync, 0);
            ImGui::SameLine();
            ImGui::RadioButton("V-Sync", &m_config.vsync, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Target Sync", &m_config.vsync, 2);
            if (m_config.vsync == 2) {
                ImGui::SliderInt("Target FPS", &m_targetFPS, 15, 360);
            }
            if (m_config.vsync != m_lastSyncType) {
                m_renderer.setVSync(m_config.vsync);
                m_lastSyncType = m_config.vsync;
            }
            ImGui::Checkbox("Animation", &m_config.animation);
            static bool fullscreen = false;
            if (ImGui::Button("Toggle Fullscreen")) {
                fullscreen = !fullscreen;
                toggleFullscreen();
            }
            ImGui::Checkbox("Auto UI Scale", &m_config.uiScaleAuto);
            if (!m_config.uiScaleAuto) {
                ImGui::SliderFloat("UI Scale", &m_config.uiScale, 0.5f, 2.5f);
            }
            if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemEdited()) {
                applyUIScale();
            }
            if (m_config.uiScaleAuto && ImGui::Button("Re-detect Scale")) {
                applyUIScale();
            }

            ImGui::EndMenu();
        }

        subAppSelectionUI();

        if (m_frameRateData.msMean > 0.0f) {
            ImGui::SameLine();
            ImGui::Text("%.1f FPS", 1000.0f / m_frameRateData.msMean);
        } else {
            ImGui::SameLine();
            ImGui::Text("--.- FPS");
        }
        ImGui::SameLine();
        ImGui::Text("%.3f ms", m_frameRateData.msMean);

        ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleVar();
}

void Application::applyUIScale() {
    if (ImGui::GetCurrentContext() == nullptr) { // ImGui not initialized yet; defer.
        return;
    }

    float scale = m_config.uiScale;
    if (m_config.uiScaleAuto && m_window) {
        float xscale = 1.0f, yscale = 1.0f;
        glfwGetWindowContentScale(m_window, &xscale, &yscale);
        scale = (xscale + yscale) * 0.5f;
        scale = glm::clamp(scale, 0.5f, 3.0f);
        m_config.uiScale = scale; // store last detected
    }
    ImGui::GetIO().FontGlobalScale = scale;
}

void Application::subAppSelectionUI() {
    if (ImGui::BeginMenu("Select SubApp")) {
        const bool noneSelected = (m_activeSubApp == nullptr);
        if (ImGui::MenuItem("<None>", nullptr, noneSelected)) {
            destroyActiveSubApp();
        }

        for (const auto &entry : SubAppRegistry::apps()) {
            const bool selected = (entry.first == m_activeSubAppName);
            if (ImGui::MenuItem(entry.first.c_str(), nullptr, selected)) {
                setSubApp(entry.first);
            }
        }
        ImGui::EndMenu();
    }
}

void Application::toggleFullscreen() {
    static int windowX = 0, windowY = 0, windowW = 0, windowH = 0;

    GLFWmonitor *monitor = glfwGetWindowMonitor(m_window);
    if (!monitor) { // Store current windowed placement
        glfwGetWindowPos(m_window, &windowX, &windowY);
        glfwGetWindowSize(m_window, &windowW, &windowH);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(
            m_window, monitor,
            0, 0, mode->width, mode->height,
            mode->refreshRate);
    } else { // Restore previous windowed placement
        glfwSetWindowMonitor(
            m_window, nullptr,
            windowX, windowY, windowW, windowH,
            0);
    }

    int w = 0, h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    if (w > 0 && h > 0) { resize({w, h}); }
}
