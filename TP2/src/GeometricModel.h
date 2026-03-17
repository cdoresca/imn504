/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#ifndef _GEOMETRIC_MODEL_H
#define _GEOMETRIC_MODEL_H

#include "GeometricModelLoader/GeometricModelLoader.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Face {
    int s1, s2, s3;
};

class GeometricModel {
public:
    GeometricModel();
    GeometricModel(std::string name, bool loadnow = true);
    ~GeometricModel();

    const std::string getName();

    int nb_vertex;
    int nb_faces;
    std::vector<glm::vec3> listVertex;
    std::vector<Face> listFaces;
    std::vector<Face> listCoordFaces;
    std::vector<glm::vec3> listNormals;
    std::vector<glm::vec3> listCoords;
    std::vector<glm::vec4> listTangents;

    static GeometricModelLoader *loader;
    bool show_interface;
    virtual void displayInterface();

protected:
    std::string m_Name;
};

#endif
