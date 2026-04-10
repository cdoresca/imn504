#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "defines.hpp"


// Freefly camera
class Camera {
public:
    Camera() = default;

    // init with position and lookAt
    void init(const Vec3f &p_position, const Vec3f &p_lookAt) {
        setPosition(p_position);
        setLookAt(p_lookAt);
        _updateVectors();
        _updateProjectionMatrix();
        _updateViewMatrix();
    }

    void displayUI();

    void handleEvents();
    void processMouseScroll(float yoffset);

    // ==================== Movement ====================

    void moveFront(const float p_amount) {_move(p_amount, -_invDir);}
    void moveRight(const float p_amount) { _move(p_amount, _right); }
    void moveUp(const float p_amount) { _move(p_amount, _up); }

    void animate(float p_deltaTime) {
        _moveTime += p_deltaTime;
        // Interpolate the camera position
        float t = glm::clamp(_moveTime / _smoothness, 0.0f, 1.0f);
        _position = glm::mix(_positionStart, _positionEnd, Vec3f(t));
        _updateVectors();
    }

    // ==================== View Control ====================

    void setLookAt(const Vec3f &p_lookAt) {
        const Vec3f newInvDir = glm::normalize(_position - p_lookAt);
        _yaw = glm::degrees(glm::atan(newInvDir.x, -newInvDir.z)) - 90; // https://en.wikipedia.org/wiki/Atan2
        _pitch = glm::degrees(glm::asin(newInvDir.y));
        rotate(0, 0); // to clamp and modulate yaw & pitch
        _updateVectors();
    }

    void rotate(const float p_yaw, const float p_pitch) {
        _yaw = glm::mod(_yaw + p_yaw, 360.f);
        _pitch = glm::clamp(_pitch + p_pitch, -89.f, 89.f);
        _updateVectors();
    }

    void rotateRelative(const float p_xRel, const float p_yRel) {
        rotate(p_xRel * _sensitivity, p_yRel * _sensitivity);
    }

    // ==================== Fields Accessors ====================

    const Mat4f &getViewMatrix() const { return _viewMatrix; }
    const Mat4f &getProjectionMatrix() const { return _projectionMatrix; }
    const Vec3f &getPosition() const { return _position; }
    const Vec3f &getFoward() const { return _position - _invDir; }
    const Vec3f &getRight() const  { return _right; }
    const Vec3f &getUp() const  { return _up; }
    float getFovy() const { return _fovy; }
    float getZNear() const { return _zNear; }
    float getZFar() const { return _zFar; }
    float getAspectRatio() const { return _aspectRatio; }
    float getSpeed() const { return _speed; }
    float getSmoothness() const { return _smoothness; }
    float getSensitivity() const { return _sensitivity; }

    // ==================== Fields Setters ====================

    void setPosition(const Vec3f &p_position) {
        _position = p_position;
        _positionStart = p_position;
        _positionEnd = p_position;
        _updateViewMatrix();
    }

    void setZNear(float p_zNear) {
        _zNear = p_zNear;
        _updateProjectionMatrix();
    }

    void setZFar(float p_zFar) {
        _zFar = p_zFar;
        _updateProjectionMatrix();
    }

    void setFovy(float p_fovy) {
        _fovy = p_fovy;
        _updateProjectionMatrix();
    }

    void resize(int p_width, int p_height) {
        _screenWidth = p_width;
        _screenHeight = p_height;
        _aspectRatio = float(_screenWidth) / float(_screenHeight);
        _updateProjectionMatrix();
    }

    void setSpeed(const float p_speed) { _speed = p_speed; }
    void setSmoothness(const float p_smoothness) { _smoothness = p_smoothness; }
    void setSensitivity(const float p_sensitivity) { _sensitivity = p_sensitivity; }

    // ==================== private methods ====================
private:

    void _move(const float p_amount, const Vec3f &p_direction) {
        const Vec3f newPos = _position + p_direction * p_amount * _speed;
        _setAnimationGoal(newPos);
        // _updateViewMatrix();
    }

    // Set the goal position for the camera animation.
    void _setAnimationGoal(const Vec3f &p_position) {
        _positionStart = _position;
        _positionEnd = p_position;
        _moveTime = 0.0f;
    }

    // update up, right and invDir vectors
    void _updateVectors() {
        const float yaw = glm::radians(_yaw);
        const float pitch = glm::radians(_pitch);

        _invDir = glm::normalize(Vec3f(glm::cos(yaw) * glm::cos(pitch),
                                  glm::sin(pitch),
                                  glm::sin(yaw) * glm::cos(pitch)));

        _right = glm::normalize(glm::cross(worldUP, _invDir)); // We suppose 'y' as up.
        _up = glm::normalize(glm::cross(_invDir, _right));

        _updateViewMatrix();
    }

    void _updateViewMatrix() {
        _viewMatrix = glm::lookAt(_position, _position - _invDir, _up);
    }

    void _updateProjectionMatrix() {
        const float fovy_radians = glm::radians(_fovy);
        _projectionMatrix = glm::perspective(fovy_radians, _aspectRatio, _zNear, _zFar);
    }

    //void handleKeyDown(SDL_Scancode scancode);

    // ==================== private fields ====================
private:
    Vec3f _position = VEC3F_ZERO;      // Position of the camera in the world
    Vec3f _positionStart = VEC3F_ZERO; // Start position during animation
    Vec3f _positionEnd = VEC3F_ZERO;   // Goal position during animation

    // Motion parameters
    Vec3f _invDir = Vec3f(0, 0, 1); // Inverse direction
    Vec3f _right = -Vec3f(1, 0, 0); // Right vector in the camera plane
    Vec3f _up = Vec3f(0, 1, 0);     // Up vector in the camera plane
    Vec3f worldUP = Vec3f(0, 1, 0); // World up vector

    // Angles defining the orientation in degrees
    float _yaw = 90.f;
    float _pitch = 0.f;

    // Screen parameters
    int _screenWidth = 1280;
    int _screenHeight = 720;
    float _aspectRatio = float(_screenWidth) / float(_screenHeight);

    // Projection parameters
    float _zNear = 0.1f;
    float _zFar = 1000.f;
    float _fovy = 60.f; // Field of view in degrees

    // Matrices
    Mat4f _viewMatrix = MAT4F_ID;
    Mat4f _projectionMatrix = MAT4F_ID;

    // Animation parameters
    float _moveTime = 0;       // Time elapsed since the start of the animation (in s)
    float _smoothness = 0.25f; // Time to reach the goal position (in s)
    float _speed = 1.f;        // Speed of the camera (unit per move call)
    float _sensitivity = 0.1f; // Sensitivity of the mouse

  

};
