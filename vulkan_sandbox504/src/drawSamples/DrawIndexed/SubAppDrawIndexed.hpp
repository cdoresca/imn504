#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

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
      
        for (int i = 0; i < 5; i++) {
            std::vector<DrawIndexedSampleData::Vertex> vertices;
            vertices.reserve(triData[i].position.size());
            for (size_t v = 0; v < triData[i].position.size(); v++) {
                vertices.push_back({Vec3f(triData[i].position[v]), Vec3f(triData[i].normal[v])});
            }

            m_vertexBuffer[i].specialUsage = RHI::Usage::eVertex;
            createAndUploadBuffer(*renderer(), m_vertexBuffer[i], vertices, "DrawIndexed Vertex Buffer");

            m_indexBuffer[i].specialUsage = RHI::Usage::eIndex;
            createAndUploadBuffer(*renderer(), m_indexBuffer[i], triData[i].indices, "DrawIndexed Index Buffer");

            m_indexCount[i] = static_cast<uint32_t>(triData[i].indices.size());
        }
    }

    void shutdown() override { renderer()->removeShaderIncludePaths(m_shaderPath); }
    void handleEvent() override { m_camera.handleEvents(); }
    void resize(int width, int height) override { m_camera.resize(width, height); }
    void animate(float deltaTime) override { m_camera.animate(deltaTime); }

    void render() override {
        m_viewUbo.updateViewUBO(m_camera);

        RHI::CommandList commandList;
        commandList.bindPipeline(&m_pipelineDesc);
        for (int i = 0; i < 5; i++) {
            commandList.useVertexBuffer(0, 0, &m_vertexBuffer[i]);
            commandList.useIndexBuffer(0, RHI::IndexType::eUint32, &m_indexBuffer[i]);
            commandList.drawIndexed(m_indexCount[i], 1, 0, 0, 0);
        }

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
    RHI::BufferDesc m_vertexBuffer[5];
    RHI::BufferDesc m_indexBuffer[5];
    uint32_t m_indexCount[5];
    std::deque<nanosecondsF> gpuTimeHistory;
    glm::mat4 brickMatrix[200];
    glm::vec4 center;
};

REGISTER_SUBAPP("DrawIndexed", SubAppDrawIndexed)


