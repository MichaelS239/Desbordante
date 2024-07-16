#pragma once

#include <cstddef>
#include <vector>

namespace util {
template <typename T>
void ReserveMore(std::vector<T>& vec, std::size_t size) {
    std::size_t old_capacity = vec.capacity();
    if (old_capacity >= size) return;
    vec.reserve(std::max(old_capacity * 2, size));
}
}  // namespace util
