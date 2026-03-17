
#include "Display.h"
#include <glm/gtc/type_ptr.hpp>

Display::Display(std::string name) : EffectGL(name) {
    vp = new GLProgram(EffectPath + "common/Effect-VS.glsl", GL_VERTEX_SHADER);
    fp = new GLProgram(EffectPath + "Display/Display-FS.glsl", GL_FRAGMENT_SHADER);

    m_ProgramPipeline->useProgramStage(vp, GL_VERTEX_SHADER_BIT);
    m_ProgramPipeline->useProgramStage(fp, GL_FRAGMENT_SHADER_BIT);
    m_ProgramPipeline->link();
    m_ProgramPipeline->printInfoLog();

    l_Texture = glGetUniformLocation(fp->getId(), "myFBO");
}

Display::~Display() {}

void Display::apply(FrameBufferObject *src, FrameBufferObject *target) {

    // note the most efficient but here for usability purposes (could be set up in the constructor if src is constant)
    glProgramUniformHandleui64ARB(fp->getId(), l_Texture, src->getColorTexture()->getHandle());

    if (target)
        target->enable();

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ProgramPipeline->bind();
    drawQuad();
    m_ProgramPipeline->release();
    if (target)
        target->disable();
}

void Display::animate(const float elapsedTime) {}
