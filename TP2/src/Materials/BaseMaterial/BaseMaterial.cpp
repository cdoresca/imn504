
#include "BaseMaterial.h"
#include "Node.h"
#include <glm/gtc/type_ptr.hpp>

BaseMaterial::BaseMaterial(std::string name) : MaterialGL(name) {

    vp = new GLProgram(MaterialPath + "BaseMaterial/Main-VS.glsl", GL_VERTEX_SHADER);
    fp = new GLProgram(MaterialPath + "BaseMaterial/Main-FS.glsl", GL_FRAGMENT_SHADER);

    m_ProgramPipeline->useProgramStage(vp, GL_VERTEX_SHADER_BIT);
    m_ProgramPipeline->useProgramStage(fp, GL_FRAGMENT_SHADER_BIT);

    l_View = glGetUniformLocation(vp->getId(), "View");
    l_Proj = glGetUniformLocation(vp->getId(), "Proj");
    l_Model = glGetUniformLocation(vp->getId(), "Model");
}

BaseMaterial::~BaseMaterial() {}

void BaseMaterial::render(Node *o) {

    m_ProgramPipeline->bind();

    o->drawGeometry(GL_TRIANGLES);
    m_ProgramPipeline->release();
}

void BaseMaterial::animate(Node *o, const float elapsedTime) {

    /**********************************************
    TP 2 - A completer
    Calculer et Transmettre les matrices Model View et Proj au shaders
    - Utilisez glm::value_ptr(mat) pour trouver le pointeur de la matrice mat a transmettre au GPU via la fonction glProgramUniform*()
    - Une matrice 4X4 se transmet grace a glProgramUniformMatrix4fv
    ***********************************************/

}
