#include "algorithms/dd/fastdd/util/evidence_inverter.h"

#include <utility>

#include <easylogging++.h>

#include "algorithms/dd/fastdd/trees/translating_tree_search.h"
#include "algorithms/dd/fastdd/trees/tree_search.h"

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
    // LOG(INFO) << "Built translating tree search";

    std::vector<boost::dynamic_bitset<>> negative_cover(bitsets_.begin(), bitsets_.end());
    // LOG(INFO) << "Created negative cover: " << bitsets_.size();
    negative_cover = MinimizeDifferentialSet(std::move(negative_cover));
    // LOG(INFO) << "Minimized differential set: " << negative_cover.size();
    positive_cover.Insert(boost::dynamic_bitset<>(df_num_));
    // LOG(INFO) << "Inserted";
    std::sort(
            negative_cover.begin(), negative_cover.end(),
            [&positive_cover](boost::dynamic_bitset<> const& a, boost::dynamic_bitset<> const& b) {
                return positive_cover.Compare(a, b);
            });
    /*LOG(INFO) << "Sorted negative cover: " << negative_cover.size();
    for (std::size_t i = 0; i != std::min(10UL, negative_cover.size()); ++i) {
        LOG(INFO) << negative_cover[i];
    }*/
    /*LOG(INFO) << "Positive cover";
    for (auto bitset : positive_cover) {
        LOG(INFO) << bitset;
    }*/
    for (std::size_t i = 0; i != negative_cover.size(); ++i) {
        // LOG(INFO) << i;
        positive_cover.HandleInvalid(negative_cover[i]);
        /*LOG(INFO) << "Positive cover";
        for (auto bitset : positive_cover) {
            LOG(INFO) << bitset;
        }*/
    }
    // LOG(INFO) << "Handled invalid";

    std::unordered_set<boost::dynamic_bitset<>> covers(positive_cover.begin(),
                                                       positive_cover.end());

    return covers;
}

std::vector<boost::dynamic_bitset<>> EvidenceInverter::MinimizeDifferentialSet(
        std::vector<boost::dynamic_bitset<>> bitsets) const {
    // LOG(INFO) << "Start minimize";
    /*for (std::size_t i = 0; i != std::min(10UL, bitsets.size()); ++i) {
        LOG(INFO) << bitsets[i];
    }*/
    std::sort(bitsets.begin(), bitsets.end(),
              [](boost::dynamic_bitset<> const& a, boost::dynamic_bitset<> const& b) {
                  int diff = b.count() - a.count();
                  return diff != 0 ? diff < 0 : b < a;
              });
    /*LOG(INFO) << "Sorted";
    for (std::size_t i = 0; i != std::min(10UL, bitsets.size()); ++i) {
        LOG(INFO) << bitsets[i];
    }*/

    TreeSearch negative_search;
    // LOG(INFO) << "Built negative search: " << bitsets.size();
    std::for_each(bitsets.begin(), bitsets.end(),
                  [&negative_search](boost::dynamic_bitset<> const& bitset) {
                      // LOG(INFO) << "Find";
                      if (!negative_search.FindSuperSet(bitset)) {
                          // LOG(INFO) << "Add";
                          negative_search.Add(bitset);
                      }
                  });
    // LOG(INFO) << "Iterate";
    std::vector<boost::dynamic_bitset<>> remaining_bitsets(negative_search.begin(),
                                                           negative_search.end());
    return remaining_bitsets;
}

}  // namespace algos::dd
