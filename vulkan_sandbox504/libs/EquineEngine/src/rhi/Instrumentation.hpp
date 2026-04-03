#pragma once
#include <chrono>
#include <vector>
#include "defines.hpp"
namespace RHI {
    struct CallStatistics{
        uint computeInvocationCount = 0;
        uint taskInvocationCount = 0;
        uint meshInvocationCount = 0;
        uint meshPrimitiveCount = 0; // count of all the meshShader emitted primitives that reached the fragment stage
        nanosecondsF gpuTime = std::chrono::nanoseconds(0);
    };

    struct PipelineStatistics{
        std::vector<CallStatistics> callStatistics;
        nanosecondsF gpuTime = std::chrono::nanoseconds(0); // sum of all the gpu time of the calls
        uint vramFootprint = 0; // sum of all the size of the resources used by the pipeline (textures, buffers, ubos)
    };

}