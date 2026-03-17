
#ifndef _ENGINE_GL_H
#define _ENGINE_GL_H

#include "NodeCollector.h"
#include "Scene.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

#include "FrameBufferObject.h"

#include "Effects/Display/Display.h"

class EngineGL {
public:
    EngineGL(int width, int height);

    ~EngineGL();

    bool init();

    bool loadScene(std::string filename);

    void render();

    void animate(const float elapsedTime);

    void onWindowResize(int w, int h);

    double getFrameTime();

    int getWidth() { return m_Width; };

    int getHeight() { return m_Height; };

    void setClearColor(glm::vec4 color);

    virtual void setupEngine();

    virtual void displayInterface();

protected:
    int m_Width;
    int m_Height;

    Scene *scene;

    NodeCollector *allNodes;

    // For TP 4
    FrameBufferObject *myFBO;
    Display *display;
};
#endif
