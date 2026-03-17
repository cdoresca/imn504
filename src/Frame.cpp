/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */
#include "Frame.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

Frame::Frame() {
    reference = NULL;
    matrix = glm::mat4(1.0f);
    isCamera = false;
}

void Frame::attachTo(Frame *f) {
    reference = f;
    if (f != NULL)
        f->m_Sons.push_back(this);
}
glm::mat4 Frame::getModelMatrix() {
    if (reference != NULL)
        return (reference->getModelMatrix() * matrix);
    else
        return (matrix);
}

glm::mat4 Frame::getMatrixCopy() {
    return (matrix);
}

Frame::~Frame() {}

void Frame::setUpFromMatrix(const glm::mat4 &m) {
    matrix = m;
    setUpdate(true);
}

void Frame::loadIdentity() {
    matrix = glm::mat4(1.0f);

    setUpdate(true);
}

// Transformations geometriques

void Frame::rotate(glm::vec3 axis, float angle) {
    matrix = glm::rotate(matrix, angle, axis);
    setUpdate(true);
}

#include <glm/gtc/quaternion.hpp>
void Frame::rotateFromQuaternion(glm::quat t) {
    glm::mat4 rotmat = glm::mat4_cast(t);
    matrix = rotmat * matrix;
    setUpdate(true);
}

void Frame::translate(glm::vec3 t) {
    matrix = glm::translate(matrix, t);
    setUpdate(true);
}

void Frame::scale(glm::vec3 s) {
    matrix = glm::scale(matrix, s);
    setUpdate(true);
}

bool Frame::isCameraFrame() {
    return isCamera;
}

void Frame::setAsCameraFrame(bool r) {
    isCamera = r;
}

glm::vec3 Frame::convertPtTo(glm::vec3 pt, Frame *to) {
    glm::vec4 pInRoot = getModelMatrix() * glm::vec4(pt, 1.0);
    glm::vec4 ret = glm::inverse(to->getModelMatrix()) * pInRoot;
    return (glm::vec3(ret));
}

glm::vec3 Frame::convertDirTo(glm::vec3 dir, Frame *to) {

    glm::mat4 T = glm::inverseTranspose(glm::inverse(to->getModelMatrix()) * getModelMatrix());
    glm::vec4 ret = T * glm::vec4(dir, 0.0);

    return (glm::vec3(ret));
}

void Frame::setUpdate(bool t) {
    m_ToUpdate = t;

    if (t && !isCameraFrame()) {
        for (int i = 0; i < m_Sons.size(); i++)
            m_Sons[i]->setUpdate(t);
    }
}

bool Frame::updateNeeded() {
    return (m_ToUpdate);
}

bool Frame::detach(Frame *f) {
    bool isInLeaves = false;
    for (int i = 0; i < f->m_Sons.size() && !isInLeaves; i++) {
        if (m_Sons[i] == f) {
            m_Sons.erase(m_Sons.begin() + i);
            return true;
        } else {
            isInLeaves = m_Sons[i]->detach(f);
        }
    }
    return isInLeaves;
}

Frame *Frame::parent() {
    return reference;
}