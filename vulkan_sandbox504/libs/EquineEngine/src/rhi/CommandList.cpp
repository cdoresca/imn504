#include "CommandList.hpp"
#include "PipelineDesc.hpp"
#include "ResourceDesc.hpp"

namespace RHI {
uint storePtr(void *ptr, std::vector<CommandParameter *> &parameters) {
    uint32_t *ptr32 = reinterpret_cast<uint32_t *>(&ptr);
    parameters.push_back(new CommandParameter(ptr32[0]));
    parameters.push_back(new CommandParameter(ptr32[1]));
    return parameters.size() - 2;
}

void *loadPtr(uint index, std::vector<CommandParameter *> &parameters) {
    return reinterpret_cast<void *>(static_cast<uintptr_t>(parameters[index]->u) | (static_cast<uintptr_t>(parameters[index + 1]->u) << 32)); // NOLINT(performance-no-int-to-ptr)
}

void *loadPtr(uint index, CommandParameter **parameters) {
    return reinterpret_cast<void *>(static_cast<uintptr_t>(parameters[index]->u) | (static_cast<uintptr_t>(parameters[index + 1]->u) << 32)); // NOLINT(performance-no-int-to-ptr)
}

// Packs raw bytes into 32-bit CommandParameters and pushes them to the vector
void storeBytes(const void *src, size_t size, std::vector<CommandParameter *> &parameters) {
    const uint8_t *byteData = static_cast<const uint8_t *>(src);

    // Calculate how many 32-bit integers we need (Ceiling division)
    size_t num32 = (size + 3) / 4;

    for (size_t i = 0; i < num32; ++i) {
        uint32_t val = 0;

        // Calculate how many bytes to copy for this chunk (handle the tail end)
        size_t remaining = size - (i * 4);
        size_t copySize = (remaining >= 4) ? 4 : remaining;

        // Use memcpy to avoid strict aliasing violations and handle alignment
        std::memcpy(&val, byteData + (i * 4), copySize);

        // Allocate and push (matches your existing architecture)
        parameters.push_back(new CommandParameter(val));
    }
}

// Reconstructs the raw bytes from the CommandParameters array into a destination buffer
void loadBytes(uint32_t index, CommandParameter **parameters, void *dst, size_t size) {
    uint8_t *byteData = static_cast<uint8_t *>(dst);
    size_t num32 = (size + 3) / 4;

    for (size_t i = 0; i < num32; ++i) {
        // parameters is CommandParameter**, so parameters[x] is a CommandParameter*
        // We assume the data was stored in the 'u' (uint32_t) member
        uint32_t val = parameters[index + i]->u;

        size_t remaining = size - (i * 4);
        size_t copySize = (remaining >= 4) ? 4 : remaining;

        std::memcpy(byteData + (i * 4), &val, copySize);
    }
}

void CommandListIR::enqueueCommand(commandFunc function, bool isPost) {
    commandQueue.push_back(function);
    isPostQueue.push_back(isPost ? 1 : 0);
}

// TODO: pas etre un animal avec des ifs dans le hotpath
bool CommandListIR::compileCommands(void *recoderDst, bool postPhase) {
    if (commandQueue.size() == 0) {
        return false; // cannot process
    }
    for (int i = 0; i < commandQueue.size(); i++) {
        if ((isPostQueue[i] != 0) == postPhase) {
            commandQueue[i](recoderDst, finalParameters.data() + offsets[i]);
        }
    }
    return true;
}

void CommandListIR::clear() {
    inputParameters.clear();
    finalParameters.clear();
    context = {};
    commandQueue.clear();
    isPostQueue.clear();
    offsets.clear();
    hash = 0;
    dynamicHash = 0;
}

void CommandList::bindPipeline(PipelineDesc *pipelineDesc, std::vector<byte> pushConstants) {
    pipelines.insert(pipelineDesc);
    commands.push_back(std::make_unique<BindPipelineCommand>(pipelineDesc, pushConstants));
    hashCombine(hash, pipelineDesc->uuid, pipelineDesc->hasTiming, pipelineDesc->hasFineInstrumentation, pipelineDesc->isDirty);
}

void CommandList::dispatchMesh(uint x, uint y, uint z) {
    commands.push_back(std::make_unique<RHICommand>(CommandType::DispatchMesh, 3, parameters.size()));
    parameters.emplace_back(x);
    parameters.emplace_back(y);
    parameters.emplace_back(z);
    hashCombine(hash, CommandType::DispatchMesh);
    hashCombine(dynamicHash, x, y, z);
}

void CommandList::dispatchMeshIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount) {
    commands.push_back(std::make_unique<IndirectMeshCommand>(CommandType::DispatchMeshIndirect, bufferDesc, offset, drawCount));
    hashCombine(hash, CommandType::DispatchMeshIndirect, offset, drawCount, bufferDesc->uuid);
}

void CommandList::dispatch(uint x, uint y, uint z) {
    commands.push_back(std::make_unique<RHICommand>(CommandType::Dispatch, 3, parameters.size()));
    parameters.emplace_back(x);
    parameters.emplace_back(y);
    parameters.emplace_back(z);
    hashCombine(hash, CommandType::Dispatch);
    hashCombine(dynamicHash, x, y, z);
}

void CommandList::dispatchIndirect(BufferDesc *bufferDesc, uint offset) {
    commands.push_back(std::make_unique<IndirectCommand>(CommandType::DispatchIndirect, bufferDesc, offset));
    hashCombine(hash, CommandType::DispatchIndirect, offset, bufferDesc->uuid);
}

void CommandList::draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) {
    commands.push_back(std::make_unique<RHICommand>(CommandType::Draw, 4, parameters.size()));
    parameters.emplace_back(vertexCount);
    parameters.emplace_back(instanceCount);
    parameters.emplace_back(firstVertex);
    parameters.emplace_back(firstInstance);
    hashCombine(hash, CommandType::Draw);
    hashCombine(dynamicHash, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::drawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance) {
    commands.push_back(std::make_unique<RHICommand>(CommandType::DrawIndexed, 5, parameters.size()));
    parameters.emplace_back(indexCount);
    parameters.emplace_back(instanceCount);
    parameters.emplace_back(firstIndex);
    parameters.emplace_back(vertexOffset);
    parameters.emplace_back(firstInstance);
    hashCombine(hash, CommandType::DrawIndexed);
    hashCombine(dynamicHash, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandList::drawIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount, uint stride) {
    commands.push_back(std::make_unique<IndirectDrawCommand>(CommandType::DrawIndirect, bufferDesc, offset, drawCount, stride));
    hashCombine(hash, CommandType::DrawIndirect, offset, drawCount, stride, bufferDesc->uuid);
}

void CommandList::drawIndexedIndirect(BufferDesc *bufferDesc, uint offset, uint drawCount, uint stride) {
    commands.push_back(std::make_unique<IndirectDrawCommand>(CommandType::DrawIndexedIndirect, bufferDesc, offset, drawCount, stride));
    hashCombine(hash, CommandType::DrawIndexedIndirect, offset, drawCount, stride, bufferDesc->uuid);
}

void CommandList::useDepthTarget(TextureDesc *renderTarget) {
    commands.push_back(std::make_unique<UseDepthTargetCommand>(renderTarget));
}

void CommandList::useRenderTarget(TextureDesc *renderTarget) {
    commands.push_back(std::make_unique<UseRenderTargetCommand>(renderTarget));
}

void CommandList::useResource(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, ResourceDesc *resourceDesc) {
    pipelines.insert(pipelineDesc);
    commands.push_back(std::make_unique<UseResourceCommand>(pipelineDesc, dstSet, dstBinding, resourceDesc));
    hashCombine(hash, pipelineDesc->uuid, dstSet, dstBinding, resourceDesc->uuid);
}

void CommandList::useVertexBuffer(uint binding, uint offset, BufferDesc *bufferDesc) {
    commands.push_back(std::make_unique<UseVertexBufferCommand>(binding, offset, bufferDesc));
    hashCombine(hash, binding, offset, bufferDesc->uuid);
}

void CommandList::useIndexBuffer(uint offset, IndexType indexType, BufferDesc *bufferDesc) {
    commands.push_back(std::make_unique<UseIndexBufferCommand>(offset, indexType, bufferDesc));
    hashCombine(hash, offset, indexType, bufferDesc->uuid);
}

void CommandList::useSampler(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, SamplerDesc *samplerDesc) {
    pipelines.insert(pipelineDesc);
    commands.push_back(std::make_unique<UseSamplerCommand>(pipelineDesc, dstSet, dstBinding, samplerDesc));
    hashCombine(hash, pipelineDesc->uuid, dstSet, dstBinding, samplerDesc->uuid);
}

void CommandList::barrier() {
    commands.push_back(std::make_unique<BarrierCommand>());
    hashCombine(hash, 576436u); // salt for now
}
} // namespace RHI

