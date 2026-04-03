#pragma once

#include <deque>
#include <numeric>
#include <string>
#include <vector>

#include "imgui.h"
#include "renderer/vulkan/VulkanRenderer.hpp"
#include "rhi/Instrumentation.hpp"
#include "rhi/PipelineDesc.hpp"
#include "rhi/ResourceDesc.hpp"

template <typename T>
void createAndUploadBuffer(VKRenderer::Renderer &renderer, RHI::BufferDesc &buffer, const std::vector<T> &data, const std::string &name) {
    glm::uint dataSize = static_cast<glm::uint>(data.size() * sizeof(T));
    buffer = RHI::BufferDesc(
        dataSize,
        buffer.shaderAccess,
        buffer.specialUsage,
        name);
    renderer.createBuffer(buffer);
    renderer.uploadToBuffer(buffer, data);
}

inline std::string formatBigNumber(uint64_t number) {
    std::string numberString = std::to_string(number);
    std::string formattedNumber;
    int count = 0;
    for (int i = int(numberString.size()) - 1; i >= 0; i--) {
        formattedNumber = numberString[i] + formattedNumber;
        count++;
        if (count % 3 == 0 && i != 0) { formattedNumber = " " + formattedNumber; }
    }
    return formattedNumber;
}

inline void displayInstrumentation(const std::string &name, RHI::PipelineDesc &pipeline, VKRenderer::Renderer *vkRenderer, std::deque<nanosecondsF> &gpuTimeHistory) {
    // Static variable to maintain its state between calls
    size_t maxHistorySize = 1000;

    std::string checkboxName = "Instrumentation" + name + ":";

    if (pipeline.hasTiming) {
        RHI::PipelineStatistics stat = vkRenderer->getPipelineStatistics(pipeline);
        gpuTimeHistory.push_back(stat.gpuTime);
        if (gpuTimeHistory.size() > maxHistorySize) { gpuTimeHistory.pop_front(); }

        nanosecondsF sum = std::accumulate(gpuTimeHistory.begin(), gpuTimeHistory.end(), nanosecondsF(0));

        ImGui::Text("GPU time (average): %f ms", std::chrono::duration_cast<millisecondsF>(sum / gpuTimeHistory.size()).count());

        ImGui::Checkbox("Fine Instrumentation", &pipeline.hasFineInstrumentation);
        if (stat.callStatistics.size() > 0 && pipeline.hasFineInstrumentation && pipeline.hasTiming) {
            // static int workgroupSizeCompute = 32;
            // static int workgoupSizeTask = 1;
            // static int workgroupSizeMesh = 32;
            // ImGui::PushItemWidth(75.0f);
            // ImGui::InputInt("A", &workgroupSizeCompute, 1);
            // ImGui::InputInt("B", &workgoupSizeTask, 1);
            // ImGui::InputInt("C", &workgroupSizeMesh, 1);
            // ImGui::PopItemWidth();
            // ImGui::Text("Compute Invocations: %d", stat.callStatistics[0].computeInvocationCount);
            // ImGui::Text("Task Invocations (wg): %s", formatBigNumber(stat.callStatistics[0].taskInvocationCount / workgoupSizeTask));
            // ImGui::Text("Mesh Invocations (wg): %s", formatBigNumber(stat.callStatistics[0].meshInvocationCount / workgroupSizeMesh));
            ImGui::Text("Mesh Primitives: %s", formatBigNumber(stat.callStatistics[0].meshPrimitiveCount).c_str());
        }

        ImGui::Separator();
    }
}