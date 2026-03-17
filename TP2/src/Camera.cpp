/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 */
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(std::string name) {
    Znear = 0.01f;
    Zfar = 10000.0f;
    aspectRatio = 1.0f;
    foV = 45.0;
    m_Frame = new Frame();
    m_Frame->setAsCameraFrame(true);
    projection_frame = new Frame();
    projection_frame->setAsCameraFrame(true);
    m_Name = name;
    needUpdate = true;
}
const std::string Camera::getName() {
    return m_Name;
}
Camera::~Camera() {
    if (m_Frame != NULL)
        delete m_Frame;
    if (projection_frame != NULL)
        delete projection_frame;
}

void Camera::setOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
    this->Znear = near;
    this->Zfar = far;

    glm::mat4 pmat = glm::ortho(left, right, bottom, top, near, far);
    projection_frame->setUpFromMatrix(pmat);
    setUpdate(true);
}

void Camera::setFrustum(float left, float right, float bottom, float top, float near, float far) {
    this->Znear = near;
    this->Zfar = far;

    glm::mat4 pmat = glm::frustum(left, right, bottom, top, near, far);
    projection_frame->setUpFromMatrix(pmat);
    setUpdate(true);
}

void Camera::setPerspectiveProjection(float foV, float aspectRatio, float near, float far) {
    this->foV = foV;
    this->aspectRatio = aspectRatio;
    this->Znear = near;
    this->Zfar = far;

    glm::mat4 projmat = glm::perspective(foV, aspectRatio, near, far);

    projection_frame->setUpFromMatrix(projmat);

    setUpdate(true);
}
void Camera::lookAt(glm::vec3 pointTo, glm::vec3 center, glm::vec3 up) {
    m_Frame->setUpFromMatrix(glm::inverse(glm::lookAt(center, pointTo, up)));

    setUpdate(true);
}

void Camera::setProjectionMatrix(glm::mat4 &m) {
    projection_frame->setUpFromMatrix(m);
}

void Camera::setUpFromMatrix(glm::mat4 &m) {
    m_Frame->setUpFromMatrix(m);
}

float Camera::getZnear() {
    return (Znear);
}

void Camera::setZnear(float n) {
    this->Znear = n;
    setPerspectiveProjection(foV, aspectRatio, Znear, Zfar);
}

float Camera::getZfar() {
    return (Zfar);
}

void Camera::setZfar(float f) {
    this->Zfar = f;
    setPerspectiveProjection(foV, aspectRatio, Znear, Zfar);
}

float Camera::getFoV() {
    return (foV);
}

void Camera::setFoV(float v) {
    this->foV = v;
    setPerspectiveProjection(foV, aspectRatio, Znear, Zfar);
}

float Camera::getAspectRatio() {
    return (aspectRatio);
}

void Camera::setAspectRatio(float a) {
    this->aspectRatio = a;
    setPerspectiveProjection(foV, aspectRatio, Znear, Zfar);
}

glm::mat4 Camera::getProjectionMatrix() {
    return *(projection_frame->getMatrix());
}

glm::mat4 Camera::getViewMatrix() {
    if (m_Frame->updateNeeded())
        viewMatrix = glm::inverse(m_Frame->getModelMatrix());

    return viewMatrix;
}

bool Camera::updateNeeded() {
    return (m_Frame->updateNeeded() || projection_frame->updateNeeded());
}

void Camera::setUpdate(bool r) {
    m_Frame->setUpdate(r);
    projection_frame->setUpdate(r);
    needUpdate = true;
}

void Camera::attachTo(Frame *f) {
    m_Frame->attachTo(f);
}

void Camera::updateBuffer() {}
