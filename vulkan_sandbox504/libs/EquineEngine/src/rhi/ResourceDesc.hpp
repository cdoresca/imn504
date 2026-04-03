#pragma once
#include "RHIdefines.hpp"
#include "UUID.hpp"
#include "defines.hpp"
#include <string>


// Ressource
/* storage_image
 * sampled_image
 * uniform_texel_buffer -> typed buffer
 * storage_texel_buffer
 * storage_buffer -> raw buffer & structured buffer
 * uniform_buffer -> constant buffer
 *
 *
 *
Buffer<float4> buffer; -> uniform_texel_buffer(R32G32B32A32_UNORM)
RWBuffer<float4> buffer;

RawAddressBuffer buffer; -> storage_buffer
struct{
    float3 a = buffer (float)
    float2 b = buffer + sizeo(a) + alignOffset;
    c;
}

structuredBuffer<struct> buffer; -> storage_buffer ->
*/

namespace RHI {
    struct ResourceDesc {
        enum class Type {
            eBuffer,
            eTexture
        };
        UUID uuid;
        std::string debugName;
        Type type;
        CPUAccessMode cpuAccess;
        ShaderAccessMode shaderAccess;
        Format format; // -> image, texel_buffer
        ResourceDesc() = default;
        ResourceDesc(Type type, ShaderAccessMode shaderAccess = ShaderAccessMode::eReadOnly, CPUAccessMode cpuAccess = CPUAccessMode::eNone, Format format = Format::eNone, const std::string& debugName = "") :
            uuid(), debugName(debugName), type(type), cpuAccess(cpuAccess), shaderAccess(shaderAccess), format(format) {
            uuid.generate();
        }
    };

    // We treat constant/uniform buffer as special, they are fully managed by the engine, thus no support for it in bufferDesc
    struct BufferDesc : ResourceDesc { // -> ssbo, ubo, storage_texel_buffer, uniform_texel_buffer
        uint size; // in bytes
        Usage specialUsage = Usage::eNone;
        BufferDesc() = default;
        BufferDesc(uint size, ShaderAccessMode shaderAccess, Usage specialUsage = Usage::eNone, const std::string& debugName = "", CPUAccessMode cpuAccess = CPUAccessMode::eNone, Format format = Format::eNone) :
            ResourceDesc(Type::eBuffer, shaderAccess, cpuAccess, format, debugName), size(size), specialUsage(specialUsage) {}
    };

    // beware input attachment are special, we do not support them (they dont have a equivalent in DirectX)
    struct TextureDesc : ResourceDesc { // -> storage_image, sampled_image
        uint width;
        uint height;
        uint depth;
        uint mipLevels;
        uint arrayLayers;
        TextureDim dim;
        bool isStorage;
        bool isImguiReady;
        // todo: color or depth target
        TextureDesc() = default;
        TextureDesc(TextureDim dim, uint width, uint height, uint depth, uint mipLevels, uint arrayLayers, Format format, ShaderAccessMode shaderAccess, const std::string &debugName = "", bool isStorage = false, CPUAccessMode cpuAccess = CPUAccessMode::eNone, bool isImguiReady = false) :
            ResourceDesc(Type::eTexture, shaderAccess, cpuAccess, format, debugName), width(width), height(height), depth(depth), mipLevels(mipLevels), arrayLayers(arrayLayers), dim(dim), isStorage(isStorage), isImguiReady(isImguiReady) {}
    };

    struct SamplerDesc {
        // todo: complete this
        // Question: should we mirror D3D12 and allow only immutable samplers?
        UUID uuid;
        FilterMode magFilter = FilterMode::eLinear;
        FilterMode minFilter = FilterMode::eLinear;
        MipMapMode mipFilter = MipMapMode::eLinear;
        AddressMode addressModeU = AddressMode::eRepeat;
        AddressMode addressModeV = AddressMode::eRepeat;
        AddressMode addressModeW = AddressMode::eRepeat;
        float mipLodBias;
        uint maxAnisotropy;
        SamplerDesc() = default;
        SamplerDesc(FilterMode magFilter, FilterMode minFilter, MipMapMode mipFilter, AddressMode addressModeU, AddressMode addressModeV, AddressMode addressModeW, float mipLodBias, uint maxAnisotropy) :
            magFilter(magFilter), minFilter(minFilter), mipFilter(mipFilter), addressModeU(addressModeU), addressModeV(addressModeV), addressModeW(addressModeW), mipLodBias(mipLodBias), maxAnisotropy(maxAnisotropy) {
            uuid.generate();
        }
    };


} // namespace RHI