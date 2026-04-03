#pragma once
#include "Commands.hpp"
#include "defines.hpp"
#include "komihash.h"

#include <memory>
#include <unordered_set>

// This was designed with 64bit pointers as a target, we do NOT support other sizes out of the box
static_assert(sizeof(void *) == 2 * sizeof(uint32_t), "Pointer size is not twice the size of uint32_t");
namespace RHI {
struct TextureDesc;
struct SamplerDesc;
struct ResourceDesc;
class PipelineDesc;

template <typename T, typename... Rest>
void hashCombine(uint64_t &seed, const T &v, const Rest &...rest) {
    seed = komihash(reinterpret_cast<const void *>(&v), sizeof(T), seed);
    (hashCombine(seed, rest), ...);
}

union CommandParameter {
    uint32_t u;
    int32_t i;
    glm::float32_t f;

    CommandParameter() : u(0) {}
    CommandParameter(uint32_t val) : u(val) {}
    CommandParameter(int32_t val) : i(val) {}
    CommandParameter(glm::float32_t val) : f(val) {}
    bool operator==(const CommandParameter &other) const {
        const float epsilon = 1e-5f;
        return (u == other.u) && (i == other.i) && (glm::abs(f - other.f) < epsilon);
    }
    bool operator!=(const CommandParameter &other) const {
        return !(*this == other);
    }
};

uint storePtr(void *ptr, std::vector<CommandParameter *> &parameters);

// We combine the lower and upper 32 bits into a 64 bit pointer
void *loadPtr(uint index, std::vector<CommandParameter *> &parameters);
void *loadPtr(uint index, CommandParameter **parameters);

void storeBytes(const void *src, size_t size, std::vector<CommandParameter *> &parameters);
void loadBytes(uint32_t index, CommandParameter **parameters, void *dst, size_t size);

struct Context {

    PipelineDesc *lastBoundPipeline = nullptr;
    bool isPostProcess = false;
    bool isRendering = false;
};

struct CommandListIR {
    using commandFunc = void (*)(void *, CommandParameter **);
    std::vector<commandFunc> commandQueue;
    std::vector<uint8_t> isPostQueue; // 0 = main, 1 = post-process
    std::vector<CommandParameter> inputParameters;
    std::vector<CommandParameter *> finalParameters;
    std::vector<uint> offsets;
    std::size_t hash = 0;
    Context context;

    size_t dynamicHash = 0;

    void enqueueCommand(commandFunc function, bool isPost);

    // When postPhase is false, execute only main commands; when true, only post-process commands
    bool compileCommands(void *recoderDst, bool postPhase);

    void clear();
};

struct CommandList {
    std::vector<std::unique_ptr<RHICommand>> commands;
    std::unordered_set<PipelineDesc *> pipelines;
    std::vector<CommandParameter> parameters;
    uint64_t hash = 0;
    uint64_t dynamicHash = 0;

    void bindPipeline(PipelineDesc *pipelineDesc, std::vector<byte> pushConstants = {});

    // TODO: check VkPhysicalDeviceMeshShaderPropertiesEXT::maxTaskWorkGroupTotalCount
    void dispatchMesh(uint x, uint y, uint z);
    void dispatchMeshIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount);
    void dispatch(uint x, uint y, uint z);
    void dispatchIndirect(BufferDesc *bufferDesc, uint offset);
    void draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance);
    void drawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance);
    void drawIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount, uint stride);
    void drawIndexedIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount, uint stride);

    void useResource(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, ResourceDesc *resourceDesc);
    void useVertexBuffer(uint binding, uint offset,
                         BufferDesc *bufferDesc);
    void useIndexBuffer(uint offset, IndexType indexType,
                        BufferDesc *bufferDesc);
    void useSampler(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, SamplerDesc *samplerDesc);
    void useRenderTarget(TextureDesc *renderTarget);
    void useDepthTarget(TextureDesc *renderTarget);
    void barrier();
};

} // namespace RHI
