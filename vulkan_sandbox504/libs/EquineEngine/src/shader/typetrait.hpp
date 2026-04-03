#pragma once
#include <vector>
#include <glm/detail/qualifier.hpp>
#include <glm/detail/setup.hpp>

// std::vector type trait
template<typename T>
struct is_vector {
    constexpr static bool value = false;
};

template<typename T>
struct is_vector<std::vector<T>> {
    constexpr static bool value = true;
    using value_type = T;
};

// glm type trait
template <typename T>
struct isGlmAggregate {
    constexpr static bool value = false;
    constexpr static glm::length_t components = 1;
    using value_type = T;
};

template <glm::length_t L, typename T, glm::qualifier Q>
struct isGlmAggregate<glm::vec<L, T, Q>> {
    constexpr static bool value = true;
    constexpr static glm::length_t components = L;
    using value_type = T;
};

template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct isGlmAggregate<glm::mat<C, R, T, Q>> {
    constexpr static bool value = true;
    constexpr static glm::length_t components = C * R;
    using value_type = T;
};