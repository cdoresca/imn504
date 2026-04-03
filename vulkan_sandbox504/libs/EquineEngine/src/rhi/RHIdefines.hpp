#pragma once
#include <cstdint>

namespace RHI {
enum StageBits : uint32_t {
    NONE = 0,
    VERTEX = 1 << 1,
    TESSELATION_CONTROL = 1 << 2,
    TESSELATION_EVALUATION = 1 << 3,
    GEOMETRY = 1 << 4,
    FRAGMENT = 1 << 5,
    COMPUTE = 1 << 6,
};
using StageMask = uint32_t;

enum class Usage {
    eNone,
    eVertex,
    eIndex,
    eIndirectArgs,
    eAccelStruct
};

enum class Format {
    eNone,
    eR32G32B32A32SF,
    eR8G8B8A8Srgb,
    eR32G32SF,
    eR32G32B32SF,
    eR32SF
};

enum class VertexInputRate {
    eVertex,
    eInstance
};

struct VertexInputBinding {
    uint32_t binding;
    uint32_t stride;
    VertexInputRate inputRate;
};

struct VertexInputAttribute {
    uint32_t location;
    uint32_t binding;
    Format format;
    uint32_t offset;
};

enum class TextureDim {
    e1D,
    e2D,
    e3D
};

enum class ShaderAccessMode {
    eNone,
    eReadOnly,
    eReadWrite
};

enum class CPUAccessMode {
    eNone,
    eWrite,
    eReadBack // -> impose host visible (idealy this would imply using/creating a staging buffer with host_cached)
};

enum class FilterMode {
    eNearest,
    eLinear
};

enum class MipMapMode {
    eNearest,
    eLinear
};

enum class AddressMode {
    eRepeat,
    eMirroredRepeat,
    eClampToEdge,
    eClampToBorder
};

enum class IndexType {
    eUint16,
    eUint32
};

struct DrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    uint32_t firstInstance;
};
} // namespace RHI
