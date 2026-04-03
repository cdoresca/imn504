#pragma once
#include "defines.hpp"
#include "typetrait.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI {
struct UBOLayout {
    enum class ManagedType {
        INT = 0,
        UINT,
        FLOAT,
        DOUBLE,
    };

    struct Field {
        ManagedType type;
        uint components = 1;
        uint count = 1;
        uint paddedSize = 0;

        bool operator==(const Field &other) const { return type == other.type && components == other.components && count == other.count; }
    };

    std::string uboName;
    std::vector<Field> fields;
    std::vector<std::string> fieldIndexToNameMap;
    uint paddedSize = 0;

    bool operator==(const UBOLayout &p_uboLayout) const {
        return uboName == p_uboLayout.uboName && fields == p_uboLayout.fields && paddedSize == p_uboLayout.paddedSize; // sizes and nameToIndexMap are not compared
    }

    bool operator!=(const UBOLayout &p_uboLayout) const { return !(*this == p_uboLayout); }
};

inline std::string managedTypeToString(const UBOLayout::ManagedType &type) {
    switch (type) {
    case UBOLayout::ManagedType::INT: return "int";
    case UBOLayout::ManagedType::UINT: return "uint";
    case UBOLayout::ManagedType::FLOAT: return "float";
    case UBOLayout::ManagedType::DOUBLE: return "double";
    default: return "unknown";
    }
}

inline bool canConvertType(const UBOLayout::ManagedType &from, const UBOLayout::ManagedType &to) {
    return (from == UBOLayout::ManagedType::INT && to == UBOLayout::ManagedType::UINT) ||
           (from == UBOLayout::ManagedType::UINT && to == UBOLayout::ManagedType::INT) || // not in GLSL spec, added for convenience (pass uint to int)
           (from == UBOLayout::ManagedType::FLOAT && to == UBOLayout::ManagedType::DOUBLE) ||
           (from == UBOLayout::ManagedType::INT && to == UBOLayout::ManagedType::DOUBLE) ||
           (from == UBOLayout::ManagedType::UINT && to == UBOLayout::ManagedType::DOUBLE) ||
           (from == UBOLayout::ManagedType::DOUBLE && to == UBOLayout::ManagedType::FLOAT) || // not in GLSL spec, added for convenience (omit f suffix)
           (from == UBOLayout::ManagedType::INT && to == UBOLayout::ManagedType::FLOAT) ||
           (from == UBOLayout::ManagedType::UINT && to == UBOLayout::ManagedType::FLOAT);
}

template <typename T>
constexpr UBOLayout::ManagedType getTypeID() {
    if constexpr (is_vector<T>::value) {
        return getTypeID<typename is_vector<T>::value_type>();
    } else if constexpr (isGlmAggregate<T>::value) {
        return getTypeID<typename isGlmAggregate<T>::value_type>();
    } else if constexpr (std::is_same_v<T, int>) {
        return UBOLayout::ManagedType::INT;
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        return UBOLayout::ManagedType::UINT;
    } else if constexpr (std::is_same_v<T, bool>) {
        return UBOLayout::ManagedType::UINT;
    } else if constexpr (std::is_same_v<T, float>) {
        return UBOLayout::ManagedType::FLOAT;
    } else if constexpr (std::is_same_v<T, double>) {
        return UBOLayout::ManagedType::DOUBLE;
    } else {
        // static_assert(false, "Unsupported type");
    }
}

struct LayoutChecker {
    const UBOLayout *layout;
    uint index = 0;
    bool valid = true;
    LayoutChecker() = default;
    LayoutChecker(const UBOLayout *layout) : layout(layout) {}

    void prepare() {
        index = 0;
        valid = true;
    }

    bool getResult() {
        if (index < layout->fields.size()) { throw std::runtime_error("Cannot get result, not iterated through all fields"); }
        return valid;
    }

    template <typename T>
    void check(T &t) {
        (void)t; // unused parameter
        using innerT = std::remove_all_extents_t<T>;
        if constexpr (std::is_array_v<T>) {
            // get rank, check
            static_assert(std::rank_v<T> == 1, "Do not support multidimensional arrays for now");
            UBOLayout::Field target = layout->fields[index];
            if (target.count != std::extent_v<T>) { throw std::runtime_error("Array count mismatch"); }
        }
        if constexpr (std::is_class_v<innerT> && !isGlmAggregate<innerT>::value) {
            // check if it is a class or struct
            throw std::runtime_error("Do not support classes or structs for now");
        }
        UBOLayout::ManagedType target = layout->fields[index].type;
        if (target == getTypeID<innerT>()) {
            index++;
            return;
        }
        throw std::runtime_error("" + layout->uboName + "type mismatch @ field " + std::to_string(index) + " expected " + managedTypeToString(target) + " got " + typeid(innerT).name()); // Beware typeid is not portable
    }
};

struct Packer {
    const UBOLayout *layout;
    std::vector<byte> *data = nullptr;
    uint offset = 0;
    uint fieldIndex = 0;

    Packer() = default;
    Packer(const UBOLayout *layout) : layout(layout) {}

    void prepare(std::vector<byte> *data) {
        this->data = data;
        offset = 0;
        fieldIndex = 0;
    }

    template <typename T>
    void pack(T &t) {
        using innerT = std::remove_all_extents_t<T>;
        if constexpr (std::is_array_v<T>) {
            // get rank, check
            if constexpr (std::rank_v<T> > 1) {
                throw std::runtime_error("Do not support multidimensional arrays for now");
            }
        }
        if constexpr (std::is_class_v<innerT> && !isGlmAggregate<innerT>::value) {
            // check if it is a class or struct
            throw std::runtime_error("Do not support classes or structs for now");
        }
        UBOLayout::Field target = layout->fields[fieldIndex];
        std::memcpy(data->data() + offset, &t, sizeof(typename isGlmAggregate<innerT>::value_type) * target.components * target.count);
        offset += target.paddedSize;
        fieldIndex++;
    }
};

struct BaseUBO {
    virtual ~BaseUBO() {}
    virtual void checkLayout(LayoutChecker &ck) = 0;
    virtual void packForUpload(Packer &p) = 0;
};

// BEWARE we are hoping that the compiler will optimize this
#define GENERATE_UBO_FUNCTIONS(...)                                 \
    void checkLayout(RHI::LayoutChecker &ck) {                      \
        auto helper = [](RHI::LayoutChecker &ck, auto &...fields) { \
            ((ck.check(fields)), ...);                              \
        };                                                          \
        std::apply(helper, std::tie(ck, __VA_ARGS__));              \
    }                                                               \
    void packForUpload(RHI::Packer &p) {                            \
        auto helper = [](RHI::Packer &p, auto &...fields) {         \
            ((p.pack(fields)), ...);                                \
        };                                                          \
        std::apply(helper, std::tie(p, __VA_ARGS__));               \
    }

class ManagedUBO {
public:
    UBOLayout layout;
    LayoutChecker layoutChecker;
    Packer packer;
    BaseUBO *uboData = nullptr;

    ManagedUBO(const UBOLayout &p_layout) : layout(p_layout) {
        layoutChecker = LayoutChecker(&layout);
        packer = Packer(&layout);
    }

    void bindUboData(BaseUBO *data) {
        layoutChecker.prepare();
        data->checkLayout(layoutChecker);
        if (!layoutChecker.getResult()) { throw std::runtime_error("UBO layout mismatch"); }
        uboData = data;
    }

    std::vector<byte> buildBufferForUpload() {
        std::vector<byte> result(layout.paddedSize);
        packer.prepare(&result);
        uboData->packForUpload(packer);
        return result;
    }
};

class PushConstant : public ManagedUBO {
public:
    vk::ShaderStageFlags stages;
    uint size = 0;
    PushConstant(const UBOLayout &p_layout) : ManagedUBO(p_layout) { size = p_layout.paddedSize; }
};
} // namespace RHI