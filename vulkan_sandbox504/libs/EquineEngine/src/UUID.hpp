#pragma once

#include <random>
//#include <xhash>

namespace UUIDHelper {
    // Beware, not clear if this is thread safe, dont care for now
    static std::random_device randomDevice;
    static std::mt19937_64 randomEngine(randomDevice());
    static std::uniform_int_distribution<uint64_t> randomDistribution;
}
constexpr static uint64_t INVALID_UUID = 0;

class UUID {
public:
    UUID() = default;
    UUID(uint64_t uuid) : _uuid(uuid) {}
    // default value of 0 is reserved for invalid UUID
    operator uint64_t() const { return _uuid; }
    void generate() {
        _uuid = UUIDHelper::randomDistribution(UUIDHelper::randomEngine); // todo: we have a better hash function, also this can generate INVALID_UUID, to fix
    }
private:
    uint64_t _uuid = INVALID_UUID;
};

namespace std { // to be used in std containers
    template<>
    struct hash<UUID> {
        std::size_t operator()(const UUID& uuid) const noexcept {
            return std::hash<uint64_t>()((uint64_t)uuid);
        }
    };
}
