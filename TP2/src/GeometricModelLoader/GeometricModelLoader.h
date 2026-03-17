#ifndef _GEOMETRICAL_MODEL_LOADER_
#define _GEOMETRICAL_MODEL_LOADER_

#include <string>
using namespace std;
class GeometricModel;

class GeometricModelLoader {
public:
    GeometricModelLoader() {};
    ~GeometricModelLoader() {};
    virtual bool loadModel(string name, GeometricModel *model) = 0;
    virtual void computeNormalAndTangents(GeometricModel *model) = 0;
    virtual void computeNormals(GeometricModel *model) = 0;
};
#endif
