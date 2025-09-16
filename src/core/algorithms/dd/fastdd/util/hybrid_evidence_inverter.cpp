#include "algorithms/dd/fastdd/util/hybrid_evidence_inverter.h"

#include <unordered_set>
#include <utility>

#include "algorithms/dd/fastdd/util/evidence_inverter.h"

namespace algos::dd {

HybridEvidenceInverter::HybridEvidenceInverter(std::vector<MatchDF> match_dfs,
                                               DifferentialFunctionBuilder const& df_builder)
    : match_dfs_(std::move(match_dfs)) {
    dif_funcs_ = df_builder.GetDifFuncs();
    dif_func_sizes_.reserve(dif_funcs_.size());
    dif_func_nums_.reserve(dif_funcs_.size() + 1);
    dif_func_nums_.push_back(0);
    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        dif_func_sizes_.push_back(dif_funcs_[i].size());
        dif_func_nums_.push_back(dif_func_nums_[i] + dif_func_sizes_[i]);
    }
    dif_func_num_ = dif_func_nums_[dif_func_nums_.size() - 1];
    dif_func_to_node_id.reserve(dif_func_num_);
    column_to_dif_funcs_.reserve(dif_funcs_.size());
    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        boost::dynamic_bitset<> cur_column_bitset(dif_func_num_);
        for (std::size_t j = 0; j != dif_funcs_[i].size(); ++j) {
            if (dif_funcs_[i][j].GetOperator() == Operator::kLessOrEqual) {
                dif_func_to_node_id.push_back(dif_func_nums_[i] + j);
            } else {
                dif_func_to_node_id.push_back(dif_func_nums_[i] + j + dif_func_num_);
            }
            cur_column_bitset.set(dif_func_nums_[i] + j);
        }
        column_to_dif_funcs_.push_back(std::move(cur_column_bitset));
    }
}

void HybridEvidenceInverter::BuildClueIndices() {
    dif_func_to_satisfied_bitsets_.reserve(dif_func_num_);
    dif_func_to_not_satisfied_bitsets_.reserve(dif_func_num_);

    for (std::size_t i = 0; i != dif_func_num_; ++i) {
        dif_func_to_satisfied_bitsets_.emplace_back(match_dfs_.size());
        dif_func_to_not_satisfied_bitsets_.emplace_back(match_dfs_.size());
    }
    for (std::size_t i = 0; i != match_dfs_.size(); ++i) {
        boost::dynamic_bitset<> diff_bitset = match_dfs_[i].GetBitset();
        for (std::size_t j = 0; j != dif_func_num_; ++j) {
            if (diff_bitset[j]) {
                dif_func_to_satisfied_bitsets_[j].set(i);
            } else {
                dif_func_to_not_satisfied_bitsets_[j].set(i);
            }
        }
    }
}

std::vector<DifferentialDependency> HybridEvidenceInverter::BuildDDs() {
    BuildClueIndices();

    std::vector<DifferentialDependency> result;

    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        for (std::size_t j = 0; j != dif_funcs_[i].size(); ++j) {
            boost::dynamic_bitset<> cur_bitset =
                    dif_func_to_not_satisfied_bitsets_[dif_func_nums_[i] + j];
            std::vector<boost::dynamic_bitset<>> cur_diff_bitsets;
            for (std::size_t index = cur_bitset.find_first();
                 index != boost::dynamic_bitset<>::npos; index = cur_bitset.find_next(index)) {
                boost::dynamic_bitset<> diff_bitset = match_dfs_[index].GetBitset();
                for (std::size_t k = dif_func_nums_[i]; k != dif_func_nums_[i + 1]; ++k) {
                    diff_bitset.set(k, false);
                }
                cur_diff_bitsets.push_back(std::move(diff_bitset));
            }

            EvidenceInverter inverter(std::move(cur_diff_bitsets), dif_func_num_,
                                      column_to_dif_funcs_, i);
            std::unordered_set<boost::dynamic_bitset<>> covers_set = inverter.GetCovers();
            std::vector<boost::dynamic_bitset<>> covers(covers_set.begin(), covers_set.end());
            std::vector<DifferentialDependency> minimized_covers = Minimize(std::move(covers));
            std::move(minimized_covers.begin(), minimized_covers.end(), std::back_inserter(result));
        }
    }

    return result;
}

}  // namespace algos::dd
