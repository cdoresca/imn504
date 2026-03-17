/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#ifndef __Node_H
#define __Node_H

#include "Frame.h"
#include "ModelGL.h"
#include <vector>

class MaterialGL;

class Node {
public:
    Node(std::string name);
    ~Node();

    Node(const Node &toCopy);

    const std::string getName();
    void setName(std::string name);

    // Render the Node and its sons using Node's material (or mat for overriding Node material)
    void render(MaterialGL *mat = NULL);

    void adopt(Node *son);
    bool disown(Node *son);

    Frame *frame();

    void setModel(ModelGL *m);
    ModelGL *getModel();

    void setMaterial(MaterialGL *r, bool recurse = false);
    MaterialGL *getMaterial();

    virtual void drawGeometry(int type = 0);
    virtual void drawGeometryInstanced(int type, int num);
    void animate(const float elapsedTime);

    std::vector<Node *> m_Sons;

    Node *getSon(std::string name);

    bool isManipulated;

    bool show_interface;
    virtual void displayInterface();

protected:
    ModelGL *m_Model;
    MaterialGL *m_Material;
    Frame *m_Frame;
    std::string m_Name;
    Node *m_Father;
};

#endif
