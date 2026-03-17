/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#include "GeometricModel.h"
#include <imgui.h>

#ifdef USE_ASSIMP
#include "GeometricModelLoader/AssimpLoader.h"
GeometricModelLoader *GeometricModel::loader = new AssimpLoader();
#else
#include "GeometricModelLoader/OBJLoader.h"
GeometricModelLoader *GeometricModel::loader = new OBJLoader();
#endif

struct triangle {
    glm::vec4 A, B, C;
};

GeometricModel::GeometricModel() {
    nb_vertex = 0;
    nb_faces = 0;
}

GeometricModel::GeometricModel(std::string name, bool loadnow) {
    nb_vertex = 0;
    nb_faces = 0;
    m_Name = name;
    show_interface = false;
    if (loadnow) {
        loader->loadModel(name, this);
    }
}

GeometricModel::~GeometricModel() {
}

const std::string GeometricModel::getName() {
    return m_Name;
}

void GeometricModel::displayInterface() {
    ImGui::Text("%s", m_Name.c_str());
    ImGui::Separator();
    ImGui::Text("Vertex Count : %lu", listVertex.size());
    ImGui::Text("Faces Count : %lu", listFaces.size());
    if (listCoords.size() > 0)
        ImGui::Text("Has Texture coordinates");
    else
        ImGui::Text("No Texture coordinates found");
}
