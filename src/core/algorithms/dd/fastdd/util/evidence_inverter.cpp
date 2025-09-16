#include "algorithms/dd/fastdd/util/evidence_inverter.h"

#include <utility>

#include "algorithms/dd/fastdd/util/translating_tree_search.h"

namespace algos::dd {

std::vector<std::size_t> EvidenceInverter::CountDFFrequencies() const {
    std::vector<std::size_t> freqs(df_num_);
    for (auto& bitset : bitsets_) {
        for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bitset.find_next(index)) {
            ++freqs[index];
        }
    }

    return freqs;
}

std::unordered_set<boost::dynamic_bitset<>> EvidenceInverter::GetCovers() const {
    TranslatingTreeSearch positive_cover(CountDFFrequencies(), column_to_dif_funcs_);

    std::vector<boost::dynamic_bitset<>> negative_cover(bitsets_.begin(), bitsets_.end());
    negative_cover = MinimizeDifferentialSet(std::move(negative_cover));

    positive_cover.Insert(boost::dynamic_bitset<>());
    std::sort(
            negative_cover.begin(), negative_cover.end(),
            [&positive_cover](boost::dynamic_bitset<> const& a, boost::dynamic_bitset<> const& b) {
                return positive_cover.Compare(a, b);
            });

    for (std::size_t i = 0; i != negative_cover.size(); ++i) {
        positive_cover.HandleInvalid(negative_cover[i]);
    }

    std::unordered_set<boost::dynamic_bitset<>> covers(positive_cover.begin(),
                                                       positive_cover.end());

    return covers;
}

std::vector<boost::dynamic_bitset<>> EvidenceInverter::MinimizeDifferentialSet(
        std::vector<boost::dynamic_bitset<>> bitsets) const {}

}  // namespace algos::dd
