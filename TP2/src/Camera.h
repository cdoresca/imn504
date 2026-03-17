
#ifndef __CAMERA__
#define __CAMERA__

#include "Frame.h"
#include <string>

struct CameraMatrices {
    glm::mat4 Proj;
    glm::mat4 View;
    glm::mat4 ViewProj;
    glm::mat4 ViewProjInv;
    glm::mat4 ViewProjNormal;
};

class Camera {
public:
    Camera(std::string name = "");

    ~Camera();

    /** Links the camera to the frame f (usually the root frame).*/
    void attachTo(Frame *f);

    //*********************** Camera position and orientation transformation**********************//

    /**
     * @brief Define the position frame, using standard glut lookAt syntax.
     * @details Place the camera frame in the virtual scene : Point the Z-axis of camera frame toward point pointTo, using center as camera frame position and up as Y vector (pointed toward the top of the screen)
     *
     * @param pointTo the 3D point faced by the camera
     * @param center the center of the camera in 3D space
     * @param up the up direction (the top of the screen) in 3D space.
     */
    void lookAt(glm::vec3 pointTo, glm::vec3 center, glm::vec3 up);

    //*********************** Camera projection transformation **********************//
    /**
     * @brief Set the projection matrix of camera
     *
     * @param m the projection matrix
     */

    void setProjectionMatrix(glm::mat4 &m);

    //*********************** Define a perspective projection *********************//

    /**
     * @brief Set Perspective projection matrix using horizontal half field of view, pixel aspect ratio (width/height ratio of one pixel),  distance to near and far clipping planes.
     *
     * @param foV to define half Field of View (in degrees) .
     * @param aspectRatio as aspect ratio.
     * @param front as distance of near clipping plane.
     * @param back as distance of back clipping plane.
     */
    void setPerspectiveProjection(float foV, float aspectRatio, float front, float back);

    /**
     * @brief Set Perspective Frustum.<a href="http://en.wikipedia.org/wiki/Viewing_frustum.">Wiki link</a>
     *
     * @param left as frustum left plane.
     * @param right as frustum right plane.
     * @param bottom as frustum bottom plane.
     * @param top as frustum top plane.
     * @param near as near clipping plane.
     * @param far as far clipping plane.
     */
    void setFrustum(float left, float right, float bottom, float top, float near, float far);

    //*********************** Define orthographic projection ********************//
    /**
     *@brief Set orthographic projection.
     *
     * @param left as frustum left plane.
     * @param right as frustum right plane.
     * @param bottom as frustum bottom plane.
     * @param top as frustum top plane.
     * @param near as near clipping plane.
     * @param far as far clipping plane.
     */
    void setOrthographicProjection(float left, float right, float bottom, float top, float near, float far);

    // *********************** Members accessors ***********************//

    /**
     * @brief Get distance of near clipping plane from camera frame center
     * @return distance of near clipping plane
     **/
    float getZnear();

    /**
     * @brief Get distance of far clipping plane from camera frame center
     * @return distance of far clipping plane
     **/
    float getZfar();

    /**
     * @brief Set distance of near clipping plane from camera frame center
     * @param distance of near clipping plane
     * @note Recompute projection matrix
     **/
    void setZnear(float n);

    /**
     * @brief Set distance of far clipping plane from camera frame center
     * @param distance of far clipping plane
     * @note Recompute projection matrix
     **/
    void setZfar(float f);

    /**
     * @brief Return field of view angle (in degrees) of camera.
     * @return field of view angle
     **/
    float getFoV();

    /**
     * @brief Set field of view angle (in degrees).
     * @param half field of view angle
     * @note Recompute projection matrix
     **/
    void setFoV(float v);

    /**
     * @brief Return pixel aspect ratio
     * @return pixel aspect ratio
     **/
    float getAspectRatio();

    /**
     * @brief Set pixel aspect ratio
     * @param pixel aspect ratio
     * @note Recompute projection matrix
     **/
    void setAspectRatio(float a);

    /**
     * @brief Return projection Matrix
     * @return projection Matrix
     **/
    glm::mat4 getProjectionMatrix();

    /**
     * @brief Return view matrix (transformation from root node to camera space)
     * @return view Matrix
     **/
    glm::mat4 getViewMatrix();

    /**
     * @brief Set up camera position and orientation from matrix m
     **/

    void setUpFromMatrix(glm::mat4 &m);

    /**
     * @brief Return true if camera moved since last update
     * @return true if camera moved since last update
     **/
    bool updateNeeded();
    /**
     * @brief Flag camera frame as having changed since last update
     * @param r for flagging frame as needing update
     **/
    virtual void setUpdate(bool r);

    /**
     * @brief return camera name
     **/
    const std::string getName();

    void updateBuffer();

    Frame *frame() { return this->m_Frame; };

protected:
    Frame *m_Frame;          ///< Camera position frame
    Frame *projection_frame; ///< Camera projection frame

    glm::mat4 viewMatrix;

    float aspectRatio; ///< pixel aspect ratio (width/height ratio of one pixel)

    float foV; ///< field of view of the camera

    float Znear; ///< distance from camera frame center to near clipping plane
    float Zfar;  ///< distance from camera frame center to far clipping plane

    std::string m_Name;

private:
    bool needUpdate;
    CameraMatrices matrices;
};

#endif
