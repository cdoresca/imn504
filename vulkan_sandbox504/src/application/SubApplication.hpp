#pragma once
#include "renderer/vulkan/VulkanRenderer.hpp"
#include <string>
#include <vector>

class SubApplication {
public:
    virtual ~SubApplication() = default;
    virtual std::string name() const = 0;
    virtual void init() = 0;
    virtual void shutdown() = 0;
    virtual void resize(int, int) {}
    virtual void update() {}
    virtual void animate(float) {}
    virtual void render() {}
    virtual void handleEvent() {}
    virtual void displayUI() {}

protected:
    VKRenderer::Renderer *renderer() const { return m_renderer; }

private:
    friend class Application;
    void attachContext(VKRenderer::Renderer *r) { m_renderer = r; }
    VKRenderer::Renderer *m_renderer = nullptr;
};

#include <functional>
#include <map>

struct SubAppRegistry {
    using Factory = std::function<std::unique_ptr<SubApplication>()>;

    static inline std::map<std::string, Factory> &apps() {
        static std::map<std::string, Factory> _apps;
        return _apps;
    }

    static inline void registerApp(const std::string &name, Factory f) {
        apps()[name] = std::move(f);
    }

    static inline std::unique_ptr<SubApplication> create(const std::string &name) {
        auto it = apps().find(name);
        if (it == apps().end() || !it->second) return {};
        return it->second(); // std::unique_ptr<SubApplication>
    }
};

#define REGISTER_SUBAPP(NAME_STRING, TYPE)                                                     \
    namespace {                                                                                \
    struct TYPE##Registrar {                                                                   \
        TYPE##Registrar() {                                                                    \
            SubAppRegistry::registerApp(NAME_STRING, [] { return std::make_unique<TYPE>(); }); \
        }                                                                                      \
    } TYPE##RegistrarInstance;                                                                 \
    }
