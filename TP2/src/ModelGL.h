/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#ifndef _MODELGL_H
#define _MODELGL_H

#include "GeometricModel.h"
#include <glad/glad.h>
#include <string>

class ModelGL {
public:
    ModelGL() = default;
    ModelGL(std::string name, bool loadnow = true);
    ~ModelGL();
    virtual void drawGeometry(GLint type = GL_TRIANGLES);
    void ModelGL::drawGeometryInstanced(GLint type, int num);
    void loadToGPU();

    std::string getName() { return m_Name; }
    bool show_interface;
    void displayInterface();
    GeometricModel *getGeometricModel() {
        return m_Model;
    }

    void rescaleModel(float scale);

protected:
    std::string m_Name;

    GeometricModel *m_Model;

    // Buffers and Arrays
    unsigned int VA_Main;
    unsigned int VBO_Vertex;
    unsigned int VBO_Faces;
    unsigned int VBO_TexCoords;
    unsigned int VBO_Normals;
    unsigned int VBO_Tangents;
    unsigned int VBO_BorderData;
};

#endif
