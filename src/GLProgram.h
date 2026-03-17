/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *  Documentation : J. Lemerle P. Le Gac
 */

#ifndef _GLPROGRAM_H
#define _GLPROGRAM_H

#include <glad/glad.h>
#include <string>

class GLProgram {
public:
    std::string info_text;

    GLProgram(std::string name, GLenum type);

    ~GLProgram();

    GLuint getId();

    bool printInfoLog();

private:
    // Attributs
    std::string m_filename;
    GLuint m_Id;   // nom associe a l'instance de GLProgram
    GLenum m_Type; // type du programme (vf, fs, gs, etc ...)
};

#endif
