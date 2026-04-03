#pragma once
#include "ResourceDesc.hpp"

#include <stdexcept>

#include "defines.hpp"

namespace RHI {
class PipelineDesc;

// Note: A parameter suffixed by ID, will be stored in the parameter list, and thus can be changed without triggering a full rebuild
enum class CommandType {
    Unknown,
    BindPipeline,
    UseResource,
    UseSampler,
    UseRenderTarget,
    UseDepthTarget,
    DispatchMesh,
    DispatchMeshIndirect,
    Dispatch,
    DispatchIndirect,
    Draw,
    DrawIndexed,
    DrawIndirect,
    DrawIndexedIndirect,
    UseVertexBuffer,
    UseIndexBuffer,
    Barrier,
};

struct RHICommand {
    const CommandType type = CommandType::Unknown;
    uint dynamicParamCount = 0;
    uint dynamicParamOffset = 0;
    RHICommand(CommandType t, uint dynamicParamCount = 0, uint dynamicParamOffset = 0) : type(t), dynamicParamCount(dynamicParamCount), dynamicParamOffset(dynamicParamOffset) {}
    virtual ~RHICommand() = default;
};

struct BindPipelineCommand : public RHICommand {
    PipelineDesc *m_pipelineDesc;
    std::vector<byte> m_pushConstantsData;
    BindPipelineCommand(PipelineDesc *pipelineDesc, std::vector<byte> pushConstantData) : RHICommand(CommandType::BindPipeline), m_pipelineDesc(pipelineDesc), m_pushConstantsData(std::move(pushConstantData)) {}
};

struct UseResourceCommand : public RHICommand {
    PipelineDesc *m_pipelineDesc;
    ResourceDesc *m_resourceDesc;
    uint m_dstSet, m_dstBinding;
    UseResourceCommand(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, ResourceDesc *resourceDesc) : RHICommand(CommandType::UseResource) {
        m_pipelineDesc = pipelineDesc;
        m_resourceDesc = resourceDesc;
        m_dstSet = dstSet;
        m_dstBinding = dstBinding;
    }
};

struct UseVertexBufferCommand : public RHICommand {
    BufferDesc *m_bufferDesc;
    uint m_binding;
    uint m_offset;
    UseVertexBufferCommand(uint binding, uint offset, BufferDesc *bufferDesc) : RHICommand(CommandType::UseVertexBuffer) {
        if (bufferDesc->specialUsage != Usage::eVertex) {
            throw std::runtime_error("UseSpecialBufferCommand can only be used with vertex buffers");
        }
        m_bufferDesc = bufferDesc;
        m_binding = binding;
        m_offset = offset;
    }
};

struct UseIndexBufferCommand : public RHICommand {
    BufferDesc *m_bufferDesc;
    uint m_offset;
    IndexType m_indexType;
    UseIndexBufferCommand(uint offset, IndexType indexType, BufferDesc *bufferDesc) : RHICommand(CommandType::UseIndexBuffer) {
        if (bufferDesc->specialUsage != Usage::eIndex) {
            throw std::runtime_error("UseIndexBufferCommand can only be used with index buffers");
        }
        m_bufferDesc = bufferDesc;
        m_offset = offset;
        m_indexType = indexType;
    }
};

struct UseSamplerCommand : public RHICommand {
    PipelineDesc *m_pipelineDesc;
    SamplerDesc *m_samplerDesc;
    uint m_dstSet, m_dstBinding;
    UseSamplerCommand(PipelineDesc *pipelineDesc, uint dstSet, uint dstBinding, SamplerDesc *samplerDesc) : RHICommand(CommandType::UseSampler) {
        m_pipelineDesc = pipelineDesc;
        m_samplerDesc = samplerDesc;
        m_dstSet = dstSet;
        m_dstBinding = dstBinding;
    }
};

struct UseRenderTargetCommand : public RHICommand {
    TextureDesc *renderTarget;
    UseRenderTargetCommand(TextureDesc *renderTarget) : RHICommand(CommandType::UseRenderTarget), renderTarget(renderTarget) {}
};

struct UseDepthTargetCommand : public RHICommand {
    TextureDesc *renderTarget;
    UseDepthTargetCommand(TextureDesc *renderTarget) : RHICommand(CommandType::UseDepthTarget), renderTarget(renderTarget) {}
};

struct BarrierCommand : public RHICommand { // big fat barrier for now
    BarrierCommand() : RHICommand(CommandType::Barrier) {};
};

struct IndirectCommand : public RHICommand {
    BufferDesc *m_bufferDesc;
    uint m_offset;
    IndirectCommand(CommandType t, BufferDesc *bufferDesc, uint offset) : RHICommand(t) {
        m_bufferDesc = bufferDesc;
        m_offset = offset;
    }
};

struct IndirectMeshCommand : public RHICommand {
    BufferDesc *m_bufferDesc;
    uint m_offset;
    uint m_drawCount;
    IndirectMeshCommand(CommandType t, BufferDesc *bufferDesc, uint offset, uint drawCount) : RHICommand(t) {
        m_bufferDesc = bufferDesc;
        m_offset = offset;
        m_drawCount = drawCount;
    }
};

struct IndirectDrawCommand : public RHICommand {
    BufferDesc *m_bufferDesc;
    uint m_offset;
    uint m_drawCount;
    uint m_stride;
    IndirectDrawCommand(CommandType t, BufferDesc *bufferDesc, uint offset, uint drawCount, uint stride) : RHICommand(t) {
        m_bufferDesc = bufferDesc;
        m_offset = offset;
        m_drawCount = drawCount;
        m_stride = stride;
    }
};

} // namespace RHI

