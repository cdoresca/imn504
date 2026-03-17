
#pragma once

#include "GLProgramPipeline.h"

#include "FrameBufferObject.h"
#include "Scene.h"

class EffectGL {
public:
    EffectGL(std::string name = "");

    virtual ~EffectGL();

    virtual void apply(FrameBufferObject *src, FrameBufferObject *target);

    virtual void animate(const float elapsedTime);

    std::string getName() { return m_Name; };

    // Interface for ImGUI
    bool show_interface;
    virtual void displayInterface(){};

protected:
    std::string m_Name;
    GLProgramPipeline *m_ProgramPipeline;
    Scene *scene;
    ModelGL *quad;

    virtual void drawQuad();
};
