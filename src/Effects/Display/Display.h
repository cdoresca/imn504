#pragma once

#include "EffectGL.h"

class Display : public EffectGL {
public:
    Display(std::string name);

    ~Display();

    virtual void apply(FrameBufferObject *src, FrameBufferObject *target);

    virtual void animate(const float elapsedTime);

    virtual void displayInterface(){};

protected:
    GLProgram *vp;
    GLProgram *fp;
    GLuint l_Texture;
};