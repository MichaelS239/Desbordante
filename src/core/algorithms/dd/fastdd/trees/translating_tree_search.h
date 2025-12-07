#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/ntree_search.h"
#include "core/algorithms/dd/fastdd/util/bitset_translator.h"

namespace algos::dd {

class TranslatingTreeSearch {
private:
    NTreeSearch tree_;
    BitsetTranslator translator_;
    std::vector<boost::dynamic_bitset<>> transformed_bitsets_;
    std::size_t bitset_size_;

    // for repeatability (ensures same order)
    boost::dynamic_bitset<> Reverse(boost::dynamic_bitset<>&& bitset) const {
        std::size_t const bitset_size = bitset.size();
        boost::dynamic_bitset<> reversed(std::move(bitset));
        for (std::size_t i = 0; i != bitset_size / 2; ++i) {
            bool temp = reversed[i];
            reversed[i] = reversed[bitset_size - 1 - i];
            reversed[bitset_size - 1 - i] = temp;
        }

        return reversed;
    }

    static boost::dynamic_bitset<> ToDynamicBitset(model::Bitset<64> const& bs, std::size_t size) {
        boost::dynamic_bitset<> bitset(size);
        for (std::size_t index = bs._Find_first(); index != 64; index = bs._Find_next(index)) {
            if (index >= size) {
                break;
            }
            bitset.set(index);
        }
        return bitset;
    }

public:
    TranslatingTreeSearch(std::vector<std::size_t> priorities,
                          std::vector<boost::dynamic_bitset<>> const& bitsets);

    void Insert(boost::dynamic_bitset<> const& bitset) {
        tree_.Insert(translator_.Transform(bitset));
    }

    bool ContainsSubset(boost::dynamic_bitset<> const& bitset) const {
        return tree_.ContainsSubset(translator_.Transform(bitset));
    }

    std::vector<boost::dynamic_bitset<>> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bitset) {
        std::vector<model::Bitset<64>> removed = tree_.GetAndRemoveGeneralizations(bitset);
        std::vector<boost::dynamic_bitset<>> retransformed;
        retransformed.reserve(removed.size());
        BitsetTranslator const& translator = translator_;
        std::ranges::transform(
                removed, std::back_inserter(retransformed), [&](model::Bitset<64> const& bitset) {
                    return translator.Retransform(ToDynamicBitset(bitset, bitset_size_));
                });
        return retransformed;
    }

    bool Compare(boost::dynamic_bitset<> const& first_bitset,
                 boost::dynamic_bitset<> const& second_bitset) const {
        int diff = second_bitset.count() - first_bitset.count();
        return diff != 0
                       ? diff < 0
                       : translator_.Transform(second_bitset) < translator_.Transform(first_bitset);
    }

    void HandleInvalid(boost::dynamic_bitset<> const& invalid_bitset);

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = boost::dynamic_bitset<>;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(NTreeSearch::Iterator it, BitsetTranslator const& translator,
                 std::size_t bitset_size)
            : it_(it), translator_(translator), bitset_size_(bitset_size) {}

        reference operator*() const {
            cur_bitset_ = translator_.Retransform(ToDynamicBitset(*it_, bitset_size_));
            return cur_bitset_;
        }

        Iterator& operator++() {
            ++it_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.it_ == b.it_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) {
            return !(a == b);
        }

    private:
        NTreeSearch::Iterator it_;
        BitsetTranslator const& translator_;
        std::size_t bitset_size_;
        boost::dynamic_bitset<> mutable cur_bitset_;  // Looks weird. Is there a better way?
    };

    Iterator begin() {
        return Iterator{tree_.begin(), translator_, bitset_size_};
    }

    Iterator end() {
        return Iterator{tree_.end(), translator_, bitset_size_};
    }
};

}  // namespace algos::dd
