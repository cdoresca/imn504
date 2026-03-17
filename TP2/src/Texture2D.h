#ifndef __TEXTURE2D__
#define __TEXTURE2D__

#include <glad/glad.h>
#include <string>

class Texture2D {
public:
    Texture2D(const std::string &filename);

    // Create an empty texture
    Texture2D(int _width = 1024, int _height = 1024);

    // Create an empty texture
    Texture2D(int _width = 1024, int _height = 1024, GLint format = GL_RGBA8);

    ~Texture2D();

    void createEmptyTexture();
    void loadToGPU();
    void makeResident();

    GLuint getId() {
        return id;
    };

    GLuint64 getHandle() {
        return handle;
    };

protected:
    GLuint id;
    GLuint64 handle;
    std::string name;
    int width, height;
    GLint format;

    unsigned char *image;
};
#endif