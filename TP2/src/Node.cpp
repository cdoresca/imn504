/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#include "Node.h"
#include "Scene.h"
#include <vector>

#include "MaterialGL.h"

Node::Node(std::string name) {
    m_Name = name;
    m_Material = NULL;
    m_Model = NULL;
    isManipulated = false;

    // Frame Creation
    m_Frame = new Frame();

    show_interface = false;
}

Node::Node(const Node &toCopy) {
    // Copy everything

    this->m_Father = (toCopy.m_Father);
    this->m_Frame = (toCopy.m_Frame);
    this->m_Material = (toCopy.m_Material);
    this->m_Model = (toCopy.m_Model);

    this->m_Name = std::string(toCopy.m_Name + "-copy");

    this->m_Sons = toCopy.m_Sons;
}

Node::~Node() {
    LOG_TRACE << "Deleting Node : " << m_Name << std::endl;

    delete m_Frame;
}

const std::string Node::getName() {
    return (m_Name);
}

void Node::setName(std::string name) {
    m_Name = name;
}

void Node::setModel(ModelGL *m) {
    m_Model = m;
}

ModelGL *Node::getModel() {
    return m_Model;
}

void Node::drawGeometry(int type) {
    m_Model->drawGeometry(type);
}
void Node::drawGeometryInstanced(int type, int num) {
    m_Model->drawGeometryInstanced(type, num);
}
void Node::setMaterial(MaterialGL *m, bool recurse) {

    m_Material = m;
    if (recurse)
        for (unsigned int i = 0; i < m_Sons.size(); i++)
            m_Sons[i]->setMaterial(m, recurse);
}

MaterialGL *Node::getMaterial() {
    return (m_Material);
}

void Node::render(MaterialGL *mat) {

    if (m_Model)
        if (mat)
            mat->render(this);
        else if (m_Material != NULL)
            m_Material->render(this);
}

void Node::animate(const float elapsedTime) {

    if (m_Material) {

        m_Material->animate(this, elapsedTime);
    }
}

void Node::adopt(Node *son) {
    son->m_Father = this;
    son->frame()->attachTo(m_Frame);
    m_Sons.push_back(son);
}

Frame *Node::frame() {
    return m_Frame;
}

bool Node::disown(Node *son) {
    bool isInLeaves = false;
    for (int i = 0; i < m_Sons.size() && !isInLeaves; i++) {
        if (m_Sons[i] == son) {
            m_Frame->detach(son->frame());
            m_Sons.erase(m_Sons.begin() + i);
            return true;
        } else {
            isInLeaves = m_Sons[i]->disown(son);
        }
    }
    return isInLeaves;
}

Node *Node::getSon(std::string name) {
    for (int i = 0; i <= m_Sons.size(); i++)
        if (m_Sons[i]->getName() == name) return m_Sons[i];

    return NULL;
}

void Node::displayInterface() {
    if (!ImGui::Begin(m_Name.c_str(), &show_interface)) {
        ImGui::End();
        return;
    }
    if (ImGui::Checkbox("Manipulate Node", &isManipulated)) {
        if (isManipulated)
            Scene::getInstance()->manipulateNode(m_Name.c_str());
        else
            Scene::getInstance()->manipulateNode("Scene");
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.06f, 0.86f, 0.05f, 0.45f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.38f, 0.98f, 0.33f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.38f, 0.98f, 0.33f, 0.30f));
    if (m_Material != NULL) {
        if (ImGui::CollapsingHeader(m_Material->getName().c_str()))
            m_Material->displayInterface();
    }
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.86f, 0.8f, 0.05f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.86f, 0.8f, 0.15f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.86f, 0.8f, 0.15f, 0.30f));
    if (m_Model != NULL) {
        std::string name = m_Model->getName();
        if (name.size() == 0)
            name = "Model";
        if (ImGui::CollapsingHeader(name.c_str()))
            m_Model->displayInterface();
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
}