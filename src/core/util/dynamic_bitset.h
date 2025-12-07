#pragma once

#include <cstddef>

#include <boost/dynamic_bitset.hpp>

#include "core/model/types/bitset.h"

namespace util {

class DynamicBitset {
private:
    std::size_t size_;
    model::Bitset<64> static_bitset_;
    boost::dynamic_bitset<> dynamic_bitset_;

public:
    static std::size_t const npos = boost::dynamic_bitset<>::npos;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size)
        : size_(size), static_bitset_(), dynamic_bitset_(size > 64 ? size - 64 : 0) {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            if (index < 64) {
                static_bitset_.set(index);
            } else {
                dynamic_bitset_.set(index - 64);
            }
        }
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        return index < 64 ? static_bitset_[index] : dynamic_bitset_[index - 64];
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        if (index < 64) {
            static_bitset_.set(index, value);
        } else {
            dynamic_bitset_.set(index - 64, value);
        }
    }

    bool none() const {
        return static_bitset_.none() && dynamic_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t FindFirst() const {
        std::size_t first = static_bitset_._Find_first();
        if (first != 64) {
            return first;
        }
        first = dynamic_bitset_.find_first();
        return first == npos ? first : first + 64;
    }

    std::size_t FindNext(std::size_t index) const {
        if (index < 64) {
            std::size_t next = static_bitset_._Find_next(index);
            if (next != 64) {
                return next;
            }
            next = dynamic_bitset_.find_first();
            return next == npos ? next : next + 64;
        }
        std::size_t next = dynamic_bitset_.find_next(index - 64);
        return next == npos ? next : next + 64;
    }

    bool operator==(DynamicBitset const& other) const = default;

    DynamicBitset& operator&=(DynamicBitset const& other) {
        static_bitset_ &= other.static_bitset_;
        dynamic_bitset_ &= other.dynamic_bitset_;
        return *this;
    }

    boost::dynamic_bitset<> ToBoostDynamicBitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = FindFirst(); index != npos; index = FindNext(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};

inline DynamicBitset operator&(DynamicBitset const& first, DynamicBitset const& second) {
    DynamicBitset result(first);
    result &= second;
    return result;
}

}  // namespace util
