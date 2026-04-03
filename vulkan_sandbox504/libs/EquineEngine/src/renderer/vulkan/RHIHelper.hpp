#pragma once
#include "EngineConfig.hpp"
#include "rhi/RHIdefines.hpp"
#include "rhi/ResourceDesc.hpp"

#include <vulkan/vulkan.hpp>

inline vk::BufferUsageFlags bufferDescToVkBufferUsage(RHI::BufferDesc desc) {
    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

    if (desc.shaderAccess != RHI::ShaderAccessMode::eNone) // TODO: should all texel buffers be storage buffers ? or should we check here for format == eNone ?
        usageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;

    if (desc.format != RHI::Format::eNone && desc.shaderAccess == RHI::ShaderAccessMode::eReadOnly)
        usageFlags |= vk::BufferUsageFlagBits::eUniformTexelBuffer;

    if (desc.format != RHI::Format::eNone && desc.shaderAccess == RHI::ShaderAccessMode::eReadWrite)
        usageFlags |= vk::BufferUsageFlagBits::eStorageTexelBuffer;

    if (desc.specialUsage == RHI::Usage::eVertex)
        usageFlags |= vk::BufferUsageFlagBits::eVertexBuffer;

    if (desc.specialUsage == RHI::Usage::eIndex)
        usageFlags |= vk::BufferUsageFlagBits::eIndexBuffer;

    if (desc.specialUsage == RHI::Usage::eIndirectArgs)
        usageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;

    if (desc.specialUsage == RHI::Usage::eAccelStruct)
        usageFlags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;

    // TODO: add more cases (ie shadertable ...)

    return usageFlags;
}

inline vk::ImageUsageFlags textureDescToVkImageUsage(RHI::TextureDesc desc) {
    vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    if (desc.shaderAccess == RHI::ShaderAccessMode::eReadWrite)
        usageFlags |= vk::ImageUsageFlagBits::eStorage;

    if (desc.shaderAccess == RHI::ShaderAccessMode::eReadOnly)
        usageFlags |= vk::ImageUsageFlagBits::eSampled;

    // TODO: color or depth target

    return usageFlags;
}

inline vk::Format formatToVkFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::eNone:
        return vk::Format::eUndefined;
    case RHI::Format::eR32G32B32A32SF:
        return vk::Format::eR32G32B32A32Sfloat;
    case RHI::Format::eR8G8B8A8Srgb:
        return vk::Format::eR8G8B8A8Srgb;
    case RHI::Format::eR32G32SF:
        return vk::Format::eR32G32Sfloat;
    case RHI::Format::eR32G32B32SF:
        return vk::Format::eR32G32B32Sfloat;
    case RHI::Format::eR32SF:
        return vk::Format::eR32Sfloat;
    }
    return {};
}

inline vk::VertexInputRate vertexInputRateToVkVertexInputRate(RHI::VertexInputRate rate) {
    switch (rate) {
    case RHI::VertexInputRate::eVertex:
        return vk::VertexInputRate::eVertex;
    case RHI::VertexInputRate::eInstance:
        return vk::VertexInputRate::eInstance;
    }
    return vk::VertexInputRate::eVertex;
}

inline vk::ImageType textureDimToVkImageType(RHI::TextureDim dim) {
    switch (dim) {
    case RHI::TextureDim::e1D:
        return vk::ImageType::e1D;
    case RHI::TextureDim::e2D:
        return vk::ImageType::e2D;
    case RHI::TextureDim::e3D:
        return vk::ImageType::e3D;
    }
    return {};
}

inline vk::ImageViewType textureDimToVkImageViewType(RHI::TextureDim dim) {
    switch (dim) {
    case RHI::TextureDim::e1D:
        return vk::ImageViewType::e1D;
    case RHI::TextureDim::e2D:
        return vk::ImageViewType::e2D;
    case RHI::TextureDim::e3D:
        return vk::ImageViewType::e3D;
        // todo: cube, array, cube array
    }
    return {};
}

inline vk::MemoryPropertyFlags cpuAccessModeToVkMemoryPropertyFlags(RHI::CPUAccessMode mode) {
    switch (mode) {
    case RHI::CPUAccessMode::eNone:
        return {};
    case RHI::CPUAccessMode::eWrite:
        return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    case RHI::CPUAccessMode::eReadBack:
        return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
    }
    return {};
}

inline vk::Filter filterModeToVkFilter(RHI::FilterMode mode) {
    switch (mode) {
    case RHI::FilterMode::eNearest:
        return vk::Filter::eNearest;
    case RHI::FilterMode::eLinear:
        return vk::Filter::eLinear;
    }
    return {};
}

inline vk::SamplerMipmapMode mipMapModeToVkSamplerMipmapMode(RHI::MipMapMode mode) {
    switch (mode) {
    case RHI::MipMapMode::eNearest:
        return vk::SamplerMipmapMode::eNearest;
    case RHI::MipMapMode::eLinear:
        return vk::SamplerMipmapMode::eLinear;
    }
    return {};
}

inline vk::SamplerAddressMode addressModeToVkSamplerAddressMode(RHI::AddressMode mode) {
    switch (mode) {
    case RHI::AddressMode::eRepeat:
        return vk::SamplerAddressMode::eRepeat;
    case RHI::AddressMode::eMirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case RHI::AddressMode::eClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    case RHI::AddressMode::eClampToBorder:
        return vk::SamplerAddressMode::eClampToBorder;
    }
    return {};
}

inline vk::IndexType indexTypeToVkIndexType(RHI::IndexType type) {
    switch (type) {
    case RHI::IndexType::eUint16:
        return vk::IndexType::eUint16;
    case RHI::IndexType::eUint32:
        return vk::IndexType::eUint32;
    }
    return vk::IndexType::eNoneKHR;
}
