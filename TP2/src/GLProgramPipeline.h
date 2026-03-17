
#ifndef _GLPROGRAMPIPELINE_H
#define _GLPROGRAMPIPELINE_H

#include <glad/glad.h>
#include <string>

#include "GLProgram.h"

class GLProgramPipeline {
public:
    GLProgramPipeline(std::string name = "");

    ~GLProgramPipeline();

    void useProgramStage(GLProgram *p, GLenum programType);
    bool link();

    void bind();
    void release();

    void printInfoLog();
    GLuint getId();

private:
    GLuint m_Pipeline;
    std::string m_Name;
    std::string info_text;
};

#endif
