#include "GLProgramPipeline.h"
#include <stdexcept>

GLProgramPipeline::GLProgramPipeline(std::string name) : m_Name(name) {
    glCreateProgramPipelines(1, &m_Pipeline);
}
GLProgramPipeline::~GLProgramPipeline() {
    glDeleteProgramPipelines(1, &m_Pipeline);
}
void GLProgramPipeline::useProgramStage(GLProgram *p, GLenum programType) {
    if (p != NULL)
        glUseProgramStages(m_Pipeline, programType, p->getId());
}

bool GLProgramPipeline::link() {
    GLint param = GL_FALSE;
    glValidateProgramPipeline(m_Pipeline);
    glGetProgramPipelineiv(m_Pipeline, GL_VALIDATE_STATUS, &param);
    if (param == GL_FALSE) {
        printInfoLog();
        throw std::logic_error("Pipeline " + m_Name + " : \n" + info_text);
    }
    return param == GL_TRUE;
}

void GLProgramPipeline::bind() {

    glBindProgramPipeline(m_Pipeline);
}
void GLProgramPipeline::release() {
    glBindProgramPipeline(0);
}
void GLProgramPipeline::printInfoLog() {

    int infologLength = 0;
    char *infoLog;

    glGetProgramPipelineiv(m_Pipeline, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        infoLog = (char *)malloc(infologLength);
        glGetProgramPipelineInfoLog(m_Pipeline, infologLength, NULL, infoLog);
        std::string df(infoLog);
        info_text += df;
        free(infoLog);
    }
}

GLuint GLProgramPipeline::getId() {
    return m_Pipeline;
}
