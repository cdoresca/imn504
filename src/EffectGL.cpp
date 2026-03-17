
#include "EffectGL.h"

EffectGL::EffectGL(std::string name) : m_Name(name), m_ProgramPipeline(NULL) {
    m_ProgramPipeline = new GLProgramPipeline(name);

    scene = Scene::getInstance();
    quad = scene->getModel<ModelGL>(ObjPath + "Quad.obj");
}

EffectGL::~EffectGL() {
    if (m_ProgramPipeline != NULL)
        delete m_ProgramPipeline;
}

void EffectGL::apply(FrameBufferObject *src, FrameBufferObject *target) {}

void EffectGL::animate(const float elapsedTime) {}

void EffectGL::drawQuad() {
    quad->drawGeometry(GL_TRIANGLES);
}
