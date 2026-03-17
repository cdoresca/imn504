#include "GeometricModelLoader/AssimpLoader.h"
#include "GeometricModel.h"

#include <stdexcept>
#ifdef USE_ASSIMP

AssimpLoader::AssimpLoader() {}

AssimpLoader::~AssimpLoader() {}

bool AssimpLoader::loadModel(string filename, GeometricModel *model) {
    // Create an instance of the Importer class
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(filename,
                                             aiProcess_CalcTangentSpace |
                                                 aiProcess_Triangulate |
                                                 aiProcess_GenSmoothNormals |
                                                 aiProcess_OptimizeMeshes |
                                                 aiProcess_JoinIdenticalVertices |
                                                 aiProcess_SortByPType);

    // If the import failed, report it
    if (!scene)
        throw std::logic_error(string("ERROR : Assimp Geometric Model Loader: ") + filename + string(" could not be opened.\n") + string(importer.GetErrorString()));

    // GetMeshes
    aiMesh **meshes = scene->mMeshes;

    // Group everithing into one mesh
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        loadFromMesh(meshes[i], model);
    }

    return true;
}

bool AssimpLoader::loadFromMesh(aiMesh *mesh, GeometricModel *model) {
    int baseVertex = model->nb_vertex;
    int baseFaces = model->nb_faces;

    model->nb_vertex += mesh->mNumVertices;
    model->nb_faces += mesh->mNumFaces;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        model->listVertex.push_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
    }

    if (mesh->HasNormals()){
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            glm::vec3 v(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            model->listNormals.push_back(v);
        }
	}

    if (mesh->HasFaces()) {
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            Face f;
            f.s1 = baseVertex + mesh->mFaces[i].mIndices[0];
            f.s2 = baseVertex + mesh->mFaces[i].mIndices[1];
            f.s3 = baseVertex + mesh->mFaces[i].mIndices[2];
            model->listFaces.push_back(f);
        }
    }

    if (mesh->HasTextureCoords(0)){
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            glm::vec3 v(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, mesh->mTextureCoords[0][i].z);
            model->listCoords.push_back(v);
        }
	}

    if (mesh->HasTangentsAndBitangents()) {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            glm::vec4 v(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f);
            model->listTangents.push_back(v);
        }
    }

    return true;
}

#endif
