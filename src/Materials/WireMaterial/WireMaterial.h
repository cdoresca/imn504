

#ifndef _WireMaterial_
#define _WireMaterial_

#include "MaterialGL.h"

class WireMaterial : public MaterialGL {
public:
    WireMaterial(std::string name = "");

    ~WireMaterial();

    virtual void render(Node *o);

    virtual void animate(Node *o, const float elapsedTime);

    virtual void displayInterface(){};

protected:
    GLProgram *vp;
    GLProgram *fp;

    GLuint l_View, l_Proj, l_Model; // location of uniforms
};

#endif