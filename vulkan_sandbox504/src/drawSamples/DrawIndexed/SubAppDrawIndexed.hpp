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


struct Box {

    glm::vec3 min;
    glm::vec3 max;
};

struct Brick {
    uint32_t type;
    glm::mat4 transform;
};

struct offset {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t count;
};
using namespace std;

class SubAppDrawIndexed : public SubApplication {
public:
    std::string name() const override { return "DrawIndexed"; }

    void init() override {
        m_camera.init(Vec3f(0, 0, 3), Vec3f(0));

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
            info[i].indexOffset = triData[i].indices.size();
            info[i].count = static_cast<uint32_t>(indices.size());
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

        
       
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
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
                Brick b;
                b.type = type;
                b.transform = T_place * T_center;
                brickMatrix.push_back(b);
            }
        }
        
            bufferMatrix.size = sizeof(Brick) * brickMatrix.size();
            bufferMatrix.shaderAccess = RHI::ShaderAccessMode::eReadOnly;
            createAndUploadBuffer(*renderer(), bufferMatrix, brickMatrix, "buffer matrix");

            for (int i = 0; i < 200; i++) {
                RHI::DrawIndexedIndirectCommand copy;
                copy.indexCount = info[brickMatrix[i].type].count;
                copy.instanceCount = 1;
                copy.firstIndex = info[brickMatrix[i].type].indexOffset;
                copy.firstInstance = 0;
                copy.vertexOffset = info[brickMatrix[i].type].vertexOffset;
                
                command.push_back(copy);
            }

            m_indirectBuffer.specialUsage = RHI::Usage::eIndirectArgs;
            createAndUploadBuffer(*renderer(), m_indirectBuffer, command, "indirect buffer");
    }

    void shutdown() override { renderer()->removeShaderIncludePaths(m_shaderPath); }
    void handleEvent() override { m_camera.handleEvents(); }
    void resize(int width, int height) override { m_camera.resize(width, height); }
    void animate(float deltaTime) override { m_camera.animate(deltaTime); }
    
    void render() override {
        m_viewUbo.updateViewUBO(m_camera);

        RHI::CommandList commandList;
        commandList.bindPipeline(&m_pipelineDesc);
        
        commandList.useResource(&m_pipelineDesc, 1, 0, &bufferMatrix);
        commandList.useVertexBuffer(0, 0, &m_vertexBuffer);
        commandList.useIndexBuffer(0, RHI::IndexType::eUint32, &m_indexBuffer);
            
        commandList.drawIndexedIndirect(&m_indirectBuffer,0,command.size(),sizeof(RHI::DrawIndexedIndirectCommand));
            

        renderer()->compileAndUseCommandList(commandList);
    }

    void displayUI() override {
        ImGui::Begin(name().c_str());
        if (ImGui::Button("Reload Shaders")) { m_pipelineDesc.reloadShaders(); }
        displayInstrumentation("DrawIndexed", m_pipelineDesc, renderer(), gpuTimeHistory);
        ImGui::End();

        m_camera.displayUI();
    }

private:
    const std::string m_shaderPath = "src/drawSamples/DrawIndexed/shaders/";
    Camera m_camera;
    DrawIndexedSampleData::ViewUBO m_viewUbo;
    RHI::PipelineDesc m_pipelineDesc;
    RHI::BufferDesc m_vertexBuffer;
    RHI::BufferDesc m_indexBuffer;
    
    std::deque<nanosecondsF> gpuTimeHistory;
    vector<Brick> brickMatrix;
    
    Box box[5];
    offset info[5];
    vector<RHI::DrawIndexedIndirectCommand> command;
    RHI::BufferDesc bufferMatrix;
    RHI::BufferDesc m_indirectBuffer;
};

REGISTER_SUBAPP("DrawIndexed", SubAppDrawIndexed)


