#ifndef _SCENE_
#define _SCENE_

#include "Camera.h"
#include "ModelGL.h"
#include "Node.h"
#include "Resource_mgr.hpp"
#include "Singleton.h"

class EffectGL;

const std::string ObjPath = "./Objets/";
const std::string MaterialPath = "./src/Materials/";
const std::string EffectPath = "./src/Effects/";

class Scene : public Singleton<Scene> {
    friend class Singleton<Scene>;

public:
    Frame *frame();
    Camera *camera();

    Node *getRoot();
    Node *getNode(std::string name);

    template <class R>
    R *getModel(std::string a) {
        return m_Models.get<R>(a);
    }

    template <class R>
    R *getNodes(std::string a) {
        return m_Nodes.get<R>(a);
    }
    template <class R>
    R *getEffect(std::string a) {
        return m_Effects.get<R>(a);
    }
    void releaseNode(std::string a);
    void releaseNode(Node *n);

    void releaseModel(std::string a);
    void releaseModel(ModelGL *m);

    void nextManipulatedNode();
    void manipulateNode(std::string name);

    void displayInterface();

    Node *getSceneNode();
    Node *getManipulatedNode();

    void resizeViewport(int w, int h);

    int getViewportWidth() { return m_width; }
    int getViewportHeight() { return m_height; }

    Resource_mgr<Node> m_Nodes;
    Resource_mgr<EffectGL> m_Effects;
    Resource_mgr<ModelGL> m_Models;
    bool needupdate;

private:
    Scene();
    ~Scene();

    Node *m_Root;
    Node *m_Scene;

    int m_width, m_height;

    Camera *current_Camera;
    Node *current_ManipulatedNode;
};

#endif