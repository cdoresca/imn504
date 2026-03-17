
#include "WireMaterial.h"
#include "Node.h"
#include <glm/gtc/type_ptr.hpp>

WireMaterial::WireMaterial(std::string name) : MaterialGL(name) {

    vp = new GLProgram(MaterialPath + "WireMaterial/Main-VS.glsl", GL_VERTEX_SHADER);
    fp = new GLProgram(MaterialPath + "WireMaterial/Main-FS.glsl", GL_FRAGMENT_SHADER);

    m_ProgramPipeline->useProgramStage(vp, GL_VERTEX_SHADER_BIT);
    m_ProgramPipeline->useProgramStage(fp, GL_FRAGMENT_SHADER_BIT);

    l_View = glGetUniformLocation(vp->getId(), "View");
    l_Proj = glGetUniformLocation(vp->getId(), "Proj");
    l_Model = glGetUniformLocation(vp->getId(), "Model");
}

WireMaterial::~WireMaterial() {}

void WireMaterial::render(Node *o) {

    m_ProgramPipeline->bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    o->drawGeometry(GL_TRIANGLES);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_ProgramPipeline->release();
}

void WireMaterial::animate(Node *o, const float elapsedTime) 
{
    glm::mat4 view = Scene::getInstance()->camera()->getViewMatrix();
    glm::mat4 proj = Scene::getInstance()->camera()->getProjectionMatrix();

    glProgramUniformMatrix4fv(vp->getId(), l_View, 1, GL_FALSE, glm::value_ptr(view));
    glProgramUniformMatrix4fv(vp->getId(), l_Proj, 1, GL_FALSE, glm::value_ptr(proj));
    glProgramUniformMatrix4fv(vp->getId(), l_Model, 1, GL_FALSE, glm::value_ptr(o->frame()->getModelMatrix()));

  

}
