

#ifdef USE_ASSIMP

#ifndef _ASSIMPLOADER_
#define _ASSIMPLOADER_

#include "GeometricModelLoader/GeometricModelLoader.h"
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
class GeometricModel;

class AssimpLoader : public GeometricModelLoader {
public:
    AssimpLoader();
    ~AssimpLoader();
    virtual bool loadModel(string name, GeometricModel *model);

protected:
    bool loadFromMesh(aiMesh *mesh, GeometricModel *model);
};

#endif

#endif
