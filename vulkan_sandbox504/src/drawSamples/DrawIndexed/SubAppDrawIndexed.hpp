#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include  <float.h>

#include "application/SubApplication.hpp"
#include "camera.hpp"
#include "rhi/PipelineDesc.hpp"
#include "utils/subAppUtils.hpp"

#include "ObjLoader.hpp"

namespace DrawIndexedSampleData {
    struct Vertex {
        Vec3f pos;
        Vec3f normal;
    };


    struct ViewUBO : public RHI::BaseUBO {
        Mat4f view;
        Mat4f projection;
        Vec4f cameraPosition;
        float near;
        float far;

        GENERATE_UBO_FUNCTIONS(view, projection, cameraPosition, near, far)

        void updateViewUBO(const Camera &camera) {
            Mat4f camera_view = camera.getViewMatrix();
            Mat4f camera_projection = camera.getProjectionMatrix();
            camera_projection[1][1] *= -1; // flip y coordinate
            view = camera_view;
            projection = camera_projection;
            cameraPosition = Vec4f(camera.getPosition(), 1);
            near = camera.getZNear();
            far = camera.getZFar();
        }
    };
} 
using namespace std;


struct aabb {

    glm::vec3 min;
    float pad;
    glm::vec3 max;
    float pads2;
    aabb transform(glm::mat4 model) {

        glm::vec3 corners[8] = {
             max,
             min,
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, min.z)
        };

        aabb tmp;
        tmp.min = glm::vec3(FLT_MAX);
        tmp.max = glm::vec3(-FLT_MAX);

        for (int i = 0; i < 8; i++) {
            glm::vec3 newCorner = model * glm::vec4(corners[i], 1);
            for (int j = 0; j < 3; j++) {
                tmp.min[j] = std::min(tmp.min[j], newCorner[j]);
                tmp.max[j] = std::max(tmp.max[j], newCorner[j]);
            }
        }
        return tmp;
    }
};


struct Brick {
    glm::mat4 transform;
    aabb box;
};

struct mur {
    vector<Brick> brick;
    uint32_t height;
    uint32_t width;
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    uint32_t countType[5];
    glm::mat4 lookAt() {
        return glm::inverse(glm::lookAt(position, center, up));

    }
    void createMur(aabb* box) {
        vector<Brick> bricksByType[5];
        glm::mat4 model = lookAt();
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int type = rand() % 5;
                glm::vec3 diff = box[type].max - box[type].min;
                glm::vec3 center = (box[type].max + box[type].min) * 0.5f;
                glm::mat4 T_center = glm::translate(glm::mat4(1.0f), -center);

                float offsetX = (i % 2 == 0) ? 0.0f : diff.x * 0.5f;
                glm::vec3 pos = glm::vec3(
                    j * diff.x + offsetX,
                    i * diff.y,
                    0.0f);

                glm::mat4 T_place = glm::translate(glm::mat4(1.0f), pos);
                glm::mat4 transform = model * T_place * T_center;
                Brick tmp {
                    transform,
                    box[type].transform(transform)
                };
                bricksByType[type].push_back(tmp);
            }
        }
        for (int i = 0; i < 5; i++) {
            brick.insert(brick.end(), bricksByType[i].begin(), bricksByType[i].end());
            countType[i] = bricksByType[i].size();
        }
       
    }
};
struct plan {
    glm::vec3 normal;
    float distance;
};

struct Frustum {
    plan face[6];

    void updateFrustrum(const Camera &camera) {
        glm::mat4 V = camera.getViewMatrix();
        glm::mat4 P = camera.getProjectionMatrix();

       

        //P[1][1] *= -1;
        glm::mat4 VP = P * V;

        
        glm::vec4 row0 = glm::vec4(VP[0][0], VP[1][0], VP[2][0], VP[3][0]);
        glm::vec4 row1 = glm::vec4(VP[0][1], VP[1][1], VP[2][1], VP[3][1]);
        glm::vec4 row2 = glm::vec4(VP[0][2], VP[1][2], VP[2][2], VP[3][2]);
        glm::vec4 row3 = glm::vec4(VP[0][3], VP[1][3], VP[2][3], VP[3][3]);

       

        // Left
        glm::vec4 left = row3 + row0;

        // Right
        glm::vec4 right = row3 - row0;

        // Bottom
        glm::vec4 bottom = row3 + row1;

        // Top
        glm::vec4 top = row3 - row1;

        // Near
        glm::vec4 nearP = row3 + row2;

        // Far
        glm::vec4 farP = row3 - row2;

        glm::vec4 planes[6] = {nearP, farP, left, right, top, bottom};

        // Normalisation
        for (int i = 0; i < 6; i++) {
            glm::vec3 normal = glm::vec3(planes[i]);
            float length = glm::length(normal);

            face[i].normal = normal / length;
            face[i].distance = planes[i].w / length;
        }
    }

    bool inside(aabb box) {
        
        for (int i = 0; i < 6; i++) {

            glm::vec3 positive;

            for (int j = 0; j < 3; j++) {
                if (face[i].normal[j] >= 0)
                    positive[j] = box.max[j];
                else
                    positive[j] = box.min[j];
            }

            float d = glm::dot(face[i].normal, positive) + face[i].distance;

            if (d < 0)
                return false;
        }
        return true;
    }
};

struct offset {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t count;
};

class SubAppDrawIndexed : public SubApplication {
public:
    std::string name() const override { return "DrawIndexed"; }

    void init() override {
        m_camera.init(Vec3f(0, 0, 3), Vec3f(0));
        m_cameraVirtuel.init(Vec3f(0, 0, 3), Vec3f(0));
        cameraSwap = &m_camera;

        renderer()->addShaderIncludePaths(m_shaderPath);
        m_pipelineDesc.init({m_shaderPath + "draw_indexed.vert", m_shaderPath + "draw_indexed.frag"});

        std::vector<RHI::VertexInputBinding> bindings = {
            {0, sizeof(DrawIndexedSampleData::Vertex), RHI::VertexInputRate::eVertex}
        };
        std::vector<RHI::VertexInputAttribute> attributes = {
            {0, 0, RHI::Format::eR32G32B32SF, (uint32_t)offsetof(DrawIndexedSampleData::Vertex, pos)},
            {1, 0, RHI::Format::eR32G32B32SF, (uint32_t)offsetof(DrawIndexedSampleData::Vertex, normal)}
        };
        m_pipelineDesc.setVertexInput(bindings, attributes);

        renderer()->createPipeline(m_pipelineDesc);
        m_pipelineDesc.setUBO(&m_viewUbo, "viewUbo");

       

        // Load a single brick model
        TriangleData triData[5] = {
            TriangleMeshLoader::loadTriangleData("models/Brick1.obj"),
            TriangleMeshLoader::loadTriangleData("models/Brick2.obj"),
            TriangleMeshLoader::loadTriangleData("models/Brick3.obj"),
            TriangleMeshLoader::loadTriangleData("models/Brick4.obj"),
            TriangleMeshLoader::loadTriangleData("models/Brick5.obj"),
        };
      
        std::vector<DrawIndexedSampleData::Vertex> vertices;
        std::vector<uint> indices;
        for (int i = 0; i < 5; i++) {

            box[i].min = glm::vec3(FLT_MAX);
            box[i].max = glm::vec3(-FLT_MAX);
            info[i].vertexOffset = vertices.size();
            info[i].indexOffset = indices.size();
            info[i].count = static_cast<uint32_t>(triData[i].indices.size());
            for (size_t v = 0; v < triData[i].position.size(); v++) {
                vertices.push_back({Vec3f(triData[i].position[v]), Vec3f(triData[i].normal[v])});

                for (int j = 0; j < 3; j++) {
                    box[i].min[j] = std::min(box[i].min[j], triData[i].position[v][j]);
                    box[i].max[j] = std::max(box[i].max[j], triData[i].position[v][j]);
                }

            }
                indices.insert(indices.end(), triData[i].indices.begin(), triData[i].indices.end());
        }
        m_vertexBuffer.specialUsage = RHI::Usage::eVertex;
        createAndUploadBuffer(*renderer(), m_vertexBuffer, vertices, "DrawIndexed Vertex Buffer");

        m_indexBuffer.specialUsage = RHI::Usage::eIndex;
        createAndUploadBuffer(*renderer(), m_indexBuffer, indices, "DrawIndexed Index Buffer");

        m_mur.position = glm::vec3(0, 0, -100);
        m_mur.center = glm::vec3(0, 0, 0);
        m_mur.up = glm::vec3(0, 1, 0);
        m_mur.width = 20;
        m_mur.height = 10;
        m_mur.createMur(box);

        bufferMatrix.size = sizeof(Brick) * m_mur.brick.size();
        bufferMatrix.shaderAccess = RHI::ShaderAccessMode::eReadOnly;
        createAndUploadBuffer(*renderer(), bufferMatrix, m_mur.brick, "buffer matrix");

        uint32_t firstInstance = 0;
        for (int i = 0; i < 5; i++) {
            RHI::DrawIndexedIndirectCommand copy;
            copy.indexCount = info[i].count;
            copy.instanceCount = m_mur.countType[i];
            copy.firstIndex = info[i].indexOffset;
            copy.firstInstance = firstInstance;
            copy.vertexOffset = info[i].vertexOffset;
                
            command.push_back(copy);
            firstInstance += copy.instanceCount;
        }

        m_indirectBuffer.specialUsage = RHI::Usage::eIndirectArgs;
        createAndUploadBuffer(*renderer(), m_indirectBuffer, command, "indirect buffer");

        visibleIDs.resize(m_mur.brick.size());

        m_visibleIDsBuffer.shaderAccess = RHI::ShaderAccessMode::eReadOnly;
        m_visibleIDsBuffer.size = sizeof(uint32_t) * visibleIDs.size();
        createAndUploadBuffer(*renderer(), m_visibleIDsBuffer, visibleIDs, "visible IDs");

    }

    void shutdown() override { renderer()->removeShaderIncludePaths(m_shaderPath); }
    void handleEvent() override { m_camera.handleEvents(); }
    void resize(int width, int height) override { m_camera.resize(width, height); }
    void animate(float deltaTime) override { m_camera.animate(deltaTime); }
    
    void render() override {
        m_viewUbo.updateViewUBO(*cameraSwap);
        m_frustum.updateFrustrum(m_camera);
        
        uint32_t globalOffset = 0;
        for (int i = 0; i < 5; i++) {
            uint32_t visibleCount = 0;
            uint32_t typeStart = 0;
            for (int k = 0; k < i; k++) typeStart += m_mur.countType[k];

            for (int j = 0; j < m_mur.countType[i]; j++) {
                if (m_frustum.inside(m_mur.brick[typeStart + j].box)) {
                    visibleIDs[globalOffset + visibleCount] = typeStart + j;
                    visibleCount++;
                }
            }
            command[i].instanceCount = visibleCount;
            command[i].firstInstance = globalOffset;
            globalOffset += visibleCount;
        }

        renderer()->uploadToBuffer(m_indirectBuffer, command);
        renderer()->uploadToBuffer(m_visibleIDsBuffer, visibleIDs);
        RHI::CommandList commandList;
        commandList.bindPipeline(&m_pipelineDesc);
        
        commandList.useResource(&m_pipelineDesc, 1, 0, &bufferMatrix);
        commandList.useResource(&m_pipelineDesc, 1, 1, &m_visibleIDsBuffer);
        commandList.useVertexBuffer(0, 0, &m_vertexBuffer);
        commandList.useIndexBuffer(0, RHI::IndexType::eUint32, &m_indexBuffer);
            
        commandList.drawIndexedIndirect(&m_indirectBuffer,0,command.size(),sizeof(RHI::DrawIndexedIndirectCommand));
            

        renderer()->compileAndUseCommandList(commandList);
    }

    void displayUI() override {
        ImGui::Begin(name().c_str());
        if (ImGui::Button("Reload Shaders")) { m_pipelineDesc.reloadShaders(); }
        if (ImGui::Button("RealCamera"))cameraSwap = &m_camera; 
        if (ImGui::Button("VirtualCamera"))cameraSwap = &m_cameraVirtuel; 
        displayInstrumentation("DrawIndexed", m_pipelineDesc, renderer(), gpuTimeHistory);
        ImGui::End();

        m_camera.displayUI();
        
    }

    

private:
    const std::string m_shaderPath = "src/drawSamples/DrawIndexed/shaders/";
    Camera m_camera;
    Camera *cameraSwap;
    Camera m_cameraVirtuel;
    DrawIndexedSampleData::ViewUBO m_viewUbo;
    RHI::PipelineDesc m_pipelineDesc;
    RHI::BufferDesc m_vertexBuffer;
    RHI::BufferDesc m_indexBuffer;
    
    std::deque<nanosecondsF> gpuTimeHistory;
    vector<uint32_t> visibleIDs; 
    RHI::BufferDesc m_visibleIDsBuffer;
    
    aabb box[5];
    offset info[5];
    vector<RHI::DrawIndexedIndirectCommand> command;
    RHI::BufferDesc bufferMatrix;
    RHI::BufferDesc m_indirectBuffer;

    mur m_mur;
    Frustum m_frustum;
};
    

REGISTER_SUBAPP("DrawIndexed", SubAppDrawIndexed)


