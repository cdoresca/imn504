#include "Texture2D.h"

#include <glm/glm.hpp>
#include <iostream>
#include <stb/stb_image.h>

Texture2D::Texture2D(const std::string &filename) : id(0), handle(0), name(filename), format(GL_RGBA8), image(NULL) {
    int channels;

    image = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (image == nullptr)
        std::cout << "Error Loading image file " << filename << std::endl;
    else
        loadToGPU();

    stbi_image_free(image);
}

Texture2D::Texture2D(int _width, int _height, GLint _format) : width(_width), height(_height), format(_format) {
    createEmptyTexture();
}

void Texture2D::createEmptyTexture() {
    int numberOfLevel = (int)(1 + glm::floor(glm::log2((float)glm::max(width, height))));
    glCreateTextures(GL_TEXTURE_2D, 1, &id);

    glTextureStorage2D(id, numberOfLevel, format, width, height);

    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateTextureMipmap(id);
    makeResident();
}

void Texture2D::loadToGPU() {
    createEmptyTexture();
    glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateTextureMipmap(id);
}

void Texture2D::makeResident() {
    handle = glGetTextureHandleARB(id);
    glMakeTextureHandleResidentARB(handle);
}

Texture2D::~Texture2D() {}
