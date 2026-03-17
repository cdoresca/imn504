#include "FrameBufferObject.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <image_DXT.h>

FrameBufferObject::FrameBufferObject(std::string name, int _width, int _height) : m_Name(name), m_Width(_width), m_Height(_height) {

    colorTexture = NULL;
    depthTexture = NULL;
    show_interface = false;
    scene = Scene::getInstance();

    glCreateFramebuffers(1, &m_FBOId);
    createTextureTargets(m_Width, m_Height);
}

FrameBufferObject::~FrameBufferObject() {
    destroy();
    glDeleteFramebuffers(1, &m_FBOId);
}

void FrameBufferObject::createTextureTargets(int width, int height) {
    m_Width = width;
    m_Height = height;

    // Texture Creation
    colorTexture = new Texture2D(m_Width, m_Height, GL_RGBA8);
    depthTexture = new Texture2D(m_Width, m_Height, GL_DEPTH_COMPONENT24);
    glNamedFramebufferTexture(m_FBOId, GL_COLOR_ATTACHMENT0, colorTexture->getId(), 0);
    glNamedFramebufferTexture(m_FBOId, GL_DEPTH_ATTACHMENT, depthTexture->getId(), 0);

    if (!CheckFramebufferStatus())
        std::cout << "Error in FBO :  " << info_text << std::endl;
}

void FrameBufferObject::resizeFBO(int width, int height) {
    destroy();
    createTextureTargets(width, height);
}

int FrameBufferObject::getWidth() {
    return m_Width;
}
int FrameBufferObject::getHeight() {
    return m_Height;
}

void FrameBufferObject::destroy() {
    delete colorTexture;
    delete depthTexture;
}

bool FrameBufferObject::CheckFramebufferStatus() {
    GLenum status;
    status = (GLenum)glCheckNamedFramebufferStatus(m_FBOId, GL_FRAMEBUFFER);
    bool evryok = false;
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        evryok = true;
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        info_text += "Unsupported framebuffer format\n";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        info_text += "Framebuffer incomplete, missing attachment\n";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        info_text += "Framebuffer incomplete, attached images must have same dimensions\n";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        info_text += "Framebuffer incomplete, attached images must have same format\n";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        info_text += "Framebuffer incomplete, missing draw buffer\n";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        info_text += "Framebuffer incomplete, missing read buffer\n";
        break;
    default:
        info_text += "Unknown Framebuffer Error";
    }
    return (evryok);
}

void FrameBufferObject::enable() {
    // Not efficient but here for code simplicity in IMN401
    glViewport(0, 0, m_Width, m_Height);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBOId);
}

void FrameBufferObject::disable() {
    // Not efficient but here for code simplicity in IMN401
    glViewport(0, 0, scene->getViewportWidth(), scene->getViewportHeight());

    glBindFramebuffer(GL_FRAMEBUFFER, NULL);
}

Texture2D *FrameBufferObject::getColorTexture() {
    return colorTexture;
}

void FrameBufferObject::writeToFile(std::string filename) {
    LOG_INFO << " Writing file" << filename << std::endl;
    int height = this->getHeight();
    int width = this->getWidth();
    int tt = height * width * 4;
    unsigned char *toto = new unsigned char[tt];
    this->enable();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, height, width, GL_RGBA, GL_UNSIGNED_BYTE, toto);
    this->disable();
    save_image_as_DDS(filename.c_str(), width, height, 4, toto);

    FILE *ff = fopen(filename.c_str(), "ab");
    fwrite(toto, sizeof(unsigned char) * tt, 1, ff);

    LOG_INFO << " File exported" << std::endl;
}
void FrameBufferObject::displayInterface() {
    if (show_interface) {
        if (!ImGui::Begin("MyFrameBuffer")) {
            ImGui::End();
            return;
        }
        int resolution = 0;

        if (ImGui::Button("Resolution"))
            ImGui::OpenPopup("selectResolution");
        ImGui::SameLine();
        std::string currentResolution = (std::to_string(m_Width) + std::string(" X ") + std::to_string(m_Height));
        // ImGui::Text(currentResolution.c_str());
        glm::ivec2 newResolution = glm::ivec2(0);
        if (ImGui::BeginPopup("selectResolution")) {
            ImGui::Text("%s", currentResolution.c_str());
            ImGui::Separator();
            if (ImGui::Selectable("64 X 64"))
                newResolution = glm::ivec2(64);
            if (ImGui::Selectable("128 X 128"))
                newResolution = glm::ivec2(128);
            if (ImGui::Selectable("256 X 256"))
                newResolution = glm::ivec2(256);
            if (ImGui::Selectable("512 X 512"))
                newResolution = glm::ivec2(512);
            if (ImGui::Selectable("1024 X 1024"))
                newResolution = glm::ivec2(1024);
            if (ImGui::Selectable("2048 X 2048"))
                newResolution = glm::ivec2(2048, 2048);
            if (ImGui::Selectable("4096 X 4096"))
                newResolution = glm::ivec2(4096, 4096);
            if (ImGui::Selectable("8192 X 8192"))
                newResolution = glm::ivec2(8192, 8192);
            ImGui::EndPopup();
        }
        static char buf[128] = "Screenshot.dds";
        bool to_save = false;

        to_save = ImGui::Button("Save");
        ImGui::SameLine();
        ImGui::Text("%s", buf);
        if (ImGui::BeginPopupContextItem("Select Filename")) {
            ImGui::InputText("", buf, 128);
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (to_save)
            writeToFile(std::string(buf));
        if (newResolution.x != 0)
            resizeFBO(newResolution.x, newResolution.y);

        int max_width = glm::min(128, m_Width);
        int max_height = glm::min(128, m_Height);
        ImTextureID tex = (void *)((uintptr_t)getColorTexture()->getId());
        ImGui::Image(tex, ImVec2((float)max_width, (float)max_height), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            max_width = glm::min(512, m_Width);
            max_height = glm::min(512, m_Height);

            ImGui::Image(tex, ImVec2((float)max_width, (float)max_height), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
            ImGui::EndTooltip();
        }
        ImGui::SameLine();

        ImGui::End();
    }
}