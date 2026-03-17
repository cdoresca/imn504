
#include "GLProgram.h"
#include "ImGuiLogger.h"
#include "utils.hpp"
#include <iostream>

GLProgram::GLProgram(std::string filename, GLenum type) : m_Type(type), m_filename(filename) {
    info_text = "";

    std::string str = readFile(filename);
    const GLchar *srcCode = str.c_str();

    m_Id = glCreateShaderProgramv(type, 1, &srcCode);

    printInfoLog();
}

GLProgram::~GLProgram() {
    glDeleteProgram(m_Id);
}
GLuint GLProgram::getId() {
    return m_Id;
}
bool GLProgram::printInfoLog() {
    GLuint isLinked;
    glGetProgramiv(m_Id, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        char *infoLog = (char *)malloc(maxLength);
        glGetProgramInfoLog(m_Id, maxLength, &maxLength, &infoLog[0]);

        std::string df(infoLog);
        info_text += df;

        LOG_ERROR << "Error in Shader " + m_filename + "\n " + info_text << std::endl;
        Logger::getInstance()->show_interface = true;
        // We don't need the program anymore.
        glDeleteProgram(m_Id);
        return false;
    }
    return true;
}
