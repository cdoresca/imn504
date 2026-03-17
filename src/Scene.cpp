#include "Scene.h"
#include "EffectGL.h"

Scene::Scene() {

    LOG_TRACE << "Creating Scene" << std::endl;
    // Get the root Node
    m_Root = m_Nodes.get("Root");

    // Create a projective default camera with standard parameter and place it in the scene

    Camera *camera = new Camera("DefaultCamera");
    camera->setPerspectiveProjection(glm::radians(45.0f), 1.77777f, 0.01f, 2000.0f);
    camera->lookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 10.0, 10.0), glm::vec3(0.0, 1.0, 0.0));
    camera->attachTo(m_Root->frame());
    current_Camera = camera;

    // Create a Scene Node and link it to root node
    m_Scene = m_Nodes.get("Scene");
    m_Root->adopt(m_Scene);

    current_ManipulatedNode = m_Scene;
}

Scene::~Scene() {
    // m_Nodes.release("Root");
}

Camera *Scene::camera() {
    return current_Camera;
}

void Scene::nextManipulatedNode() {
    current_ManipulatedNode->isManipulated = false;
    current_ManipulatedNode = m_Nodes.nextObject(current_ManipulatedNode->getName());
    if (current_ManipulatedNode == m_Root)
        current_ManipulatedNode = m_Nodes.nextObject(current_ManipulatedNode->getName());
    current_ManipulatedNode->isManipulated = true;
    LOG_INFO << "manipulating Node " << current_ManipulatedNode->getName() << std::endl;
}
void Scene::manipulateNode(std::string name) {
    Node *c_node = m_Nodes.find(name);
    if (c_node == NULL)
        LOG_WARNING << "Error : Node " << name << " does not exists." << std::endl;
    else {
        if (current_ManipulatedNode != NULL)
            current_ManipulatedNode->isManipulated = false;
        current_ManipulatedNode = c_node;
        c_node->isManipulated = true;
        LOG_INFO << "Manipulated node is now " << current_ManipulatedNode->getName() << std::endl;
    }
}

Node *Scene::getSceneNode() {
    return m_Scene;
}
Node *Scene::getManipulatedNode() {
    return current_ManipulatedNode;
}
Frame *Scene::frame() {
    return m_Root->frame();
}
Node *Scene::getRoot() {
    return m_Root;
}
Node *Scene::getNode(std::string name) {
    return (m_Nodes.get(name));
}

void Scene::releaseNode(Node *n) {
    m_Nodes.release(n->getName());
}

void Scene::releaseNode(std::string name) {
    m_Nodes.release(name);
}

void Scene::releaseModel(std::string a) {
    m_Models.release(a);
}

void Scene::releaseModel(ModelGL *m) {
    m_Models.release(m->getName());
}

void Scene::resizeViewport(int w, int h) {
    m_width = w;
    m_height = h;
}

void Scene::displayInterface() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Scene Information")) {
            if (ImGui::BeginMenu("Nodes")) {
                for (int i = 0; i < m_Nodes.size(); i++)
                    ImGui::MenuItem(m_Nodes.get(i)->getName().c_str(), NULL, &(m_Nodes.get(i)->show_interface));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Effects")) {
                for (int i = 0; i < m_Effects.size(); i++)
                    ImGui::MenuItem(m_Effects.get(i)->getName().c_str(), NULL, &(m_Effects.get(i)->show_interface));
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        for (int i = 0; i < m_Nodes.size(); i++)
            if (m_Nodes.get(i)->show_interface)
                m_Nodes.get(i)->displayInterface();

        for (int i = 0; i < m_Effects.size(); i++)
            if (m_Effects.get(i)->show_interface)
                m_Effects.get(i)->displayInterface();

        ImGui::EndMainMenuBar();
    }

    if (!ImGui::Begin("Manipulated Nodes")) {
        ImGui::End();
        return;
    }

    for (int i = 0; i < m_Nodes.size(); i++) {
        Node *n = m_Nodes.get(i);

        if (ImGui::RadioButton(n->getName().c_str(), current_ManipulatedNode == n)) {
            manipulateNode(n->getName());
        }
    }

    ImGui::End();
}