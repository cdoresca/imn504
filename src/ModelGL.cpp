#include "ModelGL.h"
#include <glm/glm.hpp>

ModelGL::ModelGL(std::string name, bool loadnow) {
    this->m_Name = name;

    m_Model = new GeometricModel(name, loadnow);

    if (loadnow)
        loadToGPU();
}

ModelGL::~ModelGL() {

    glDeleteBuffers(1, &VBO_Vertex);
    glDeleteBuffers(1, &VBO_Normals);
    glDeleteBuffers(1, &VBO_Faces);
    glDeleteBuffers(1, &VBO_TexCoords);
    glDeleteBuffers(1, &VBO_Tangents);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VA_Main);
}

void ModelGL::loadToGPU() {
    if (m_Model != NULL) {
        glCreateVertexArrays(1, &VA_Main);

        // Create VBO and Send data to GPU
        if (m_Model->listVertex.size() > 0) {
            glCreateBuffers(1, &VBO_Vertex);
            glNamedBufferData(VBO_Vertex, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listVertex.front()), GL_STATIC_DRAW);
            glEnableVertexArrayAttrib(VA_Main, 0);
            glVertexArrayAttribFormat(VA_Main, 0, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayVertexBuffer(VA_Main, 0, VBO_Vertex, 0, sizeof(glm::vec3));
            glVertexArrayAttribBinding(VA_Main, 0, 0);
        }

        if (m_Model->listNormals.size() > 0) {
            glCreateBuffers(1, &VBO_Normals);
            glNamedBufferData(VBO_Normals, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listNormals.front()), GL_STATIC_DRAW_ARB);
            glEnableVertexArrayAttrib(VA_Main, 2);
            glVertexArrayAttribFormat(VA_Main, 2, 3, GL_FLOAT, GL_TRUE, 0);
            glVertexArrayVertexBuffer(VA_Main, 2, VBO_Normals, 0, sizeof(glm::vec3));
            glVertexArrayAttribBinding(VA_Main, 2, 2);
        }
        if (m_Model->listCoords.size() != 0) {

            glCreateBuffers(1, &VBO_TexCoords);
            glNamedBufferData(VBO_TexCoords, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listCoords.front()), GL_STATIC_DRAW_ARB);
            glEnableVertexArrayAttrib(VA_Main, 3);
            glVertexArrayAttribFormat(VA_Main, 3, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayVertexBuffer(VA_Main, 3, VBO_TexCoords, 0, sizeof(glm::vec3));
            glVertexArrayAttribBinding(VA_Main, 3, 3);
        }

        if (m_Model->listTangents.size() > 0) {
            glCreateBuffers(1, &VBO_Tangents);
            glNamedBufferData(VBO_Tangents, m_Model->nb_vertex * sizeof(glm::vec4), &(m_Model->listTangents.front()), GL_STATIC_DRAW_ARB);
            glEnableVertexArrayAttrib(VA_Main, 4);
            glVertexArrayAttribFormat(VA_Main, 4, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayVertexBuffer(VA_Main, 4, VBO_Tangents, 0, sizeof(glm::vec4));
            glVertexArrayAttribBinding(VA_Main, 4, 4);
        }

        if (m_Model->listFaces.size() > 0) {
            glCreateBuffers(1, &VBO_Faces);
            glNamedBufferData(VBO_Faces, m_Model->nb_faces * sizeof(Face), &(m_Model->listFaces.front()), GL_STATIC_DRAW_ARB);
        }

        glBindVertexArray(VA_Main);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_Faces);

        glBindVertexArray(0);
    }
}

void ModelGL::drawGeometry(GLint type) {
    if (m_Model != NULL) {
        glBindVertexArray(VA_Main);

        glDrawRangeElements(type, 0, 3 * m_Model->nb_faces, 3 * m_Model->nb_faces, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }
}

void ModelGL::drawGeometryInstanced(GLint type, int num) {
    if (m_Model != NULL) {
        glBindVertexArray(VA_Main);

        glDrawElementsInstanced(GL_TRIANGLES, 3 * m_Model->nb_faces, GL_UNSIGNED_INT, 0, num);
        glBindVertexArray(0);
    }
}
void ModelGL::displayInterface() {
    m_Model->displayInterface();
}


void ModelGL::rescaleModel(float scale)
{

    for (int i = 0; i < m_Model->nb_vertex; i++)
        m_Model->listVertex[i] *= scale;

     glNamedBufferData(VBO_Vertex, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listVertex.front()), GL_STATIC_DRAW);
}