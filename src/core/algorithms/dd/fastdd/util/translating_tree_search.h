#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/dd/fastdd/util/bitset_translator.h"
#include "util/ntree_search.h"

namespace algos::dd {

class TranslatingTreeSearch {
private:
    util::NTreeSearch tree_;
    BitsetTranslator translator_;
    std::vector<boost::dynamic_bitset<>> transformed_bitsets_;

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
        std::vector<boost::dynamic_bitset<>> removed = tree_.GetAndRemoveGeneralizations(bitset);
        std::vector<boost::dynamic_bitset<>> retransformed;
        retransformed.reserve(removed.size());
        BitsetTranslator const& translator = translator_;
        std::transform(removed.begin(), removed.end(), retransformed.begin(),
                       [&translator](boost::dynamic_bitset<> const& bitset) {
                           return translator.Retransform(bitset);
                       });
        return retransformed;
    }

    bool Compare(boost::dynamic_bitset<> const& first_bitset,
                 boost::dynamic_bitset<> const& second_bitset) const {
        int diff = second_bitset.count() - first_bitset.count();
        return diff != 0
                       ? diff
                       : translator_.Transform(second_bitset) < translator_.Transform(first_bitset);
    }

    void HandleInvalid(boost::dynamic_bitset<> const& invalid_bitset);

    util::NTreeSearch::Iterator begin() {
        return tree_.begin();
    }

    util::NTreeSearch::Iterator end() {
        return tree_.end();
    }
};

}  // namespace algos::dd
