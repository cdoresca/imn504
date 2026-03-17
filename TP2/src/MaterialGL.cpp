
#include "MaterialGL.h"
#include "Node.h"

MaterialGL::MaterialGL(std::string name) : m_Name(name), m_ProgramPipeline(NULL) {
    m_ProgramPipeline = new GLProgramPipeline(name);

    scene = Scene::getInstance();
}

MaterialGL::~MaterialGL() {
    if (m_ProgramPipeline != NULL)
        delete m_ProgramPipeline;
}

void MaterialGL::render(Node *o) {}

void MaterialGL::animate(Node *o, const float elapsedTime) {}
