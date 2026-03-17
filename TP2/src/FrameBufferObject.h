#pragma once

#include "Scene.h"
#include "Texture2D.h"
#include <string>

class FrameBufferObject {
public:
    FrameBufferObject(std::string name = "", int _width = 1024, int _height = 1024);

    ~FrameBufferObject();

    GLuint getId() { return m_FBOId; };

    /** @brief Bind the FBO as rendering target. all subsequent rendering command will draw into the corresponding color attachment. Change the viewport to match the FBO size (Note : Changing viewport here is not the most efficient strategy but improves usability in IMN401) */
    void enable();

    /** @brief Release the FBO as rendering target.Bind NULL as rendering target (initial default state). Change the viewport to match the screen size (Note : Changing viewport here is not the most efficient strategy but improves usability in IMN401) */
    void disable();

    /** @brief Destroy FBO in GPU memory but keep the C++ object intact. Usage except in the destructor mostly for changing the FBO parameters(size, attachments... without losing the c++ pointer and FBO Id) */
    void destroy();

    /** @brief Create simple texture attachments (color, depth) of given size for the current FBO*/
    void createTextureTargets(int width, int height);

    void resizeFBO(int width, int height);

    /**
     * @brief Return the color attachment as a Texture2D
     * @return The color attachment
     **/
    Texture2D *getColorTexture();

    int getWidth();
    int getHeight();

    std::string getName() {
        return m_Name;
    }

    // Interface - ImGUI
    bool show_interface;
    void writeToFile(std::string filename);
    virtual void displayInterface();

private:
    int m_Width;
    int m_Height;

    std::string m_Name;
    std::string info_text;
    GLuint m_FBOId;
    bool CheckFramebufferStatus();

    Scene *scene;

    Texture2D *colorTexture;
    Texture2D *depthTexture;
};