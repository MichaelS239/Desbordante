#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/index.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

class SimilarityData {
public:
    using ColMatchesInfo = std::vector<
            std::tuple<std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                       model::Index, model::Index>>;

private:
    indexes::RecordsInfo const* const records_info_;

    std::vector<ColumnMatchInfo> const column_matches_sim_info_;
    std::vector<LhsCCVIdsInfo> const column_matches_lhs_ids_info_;

    std::vector<model::Index> const sorted_to_original_;

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return records_info_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return records_info_->GetRightCompressor();
    }

public:
    SimilarityData(indexes::RecordsInfo* records_info,
                   std::vector<ColumnMatchInfo> column_matches_sim_info,
                   std::vector<LhsCCVIdsInfo> column_matches_lhs_ids_info,
                   std::vector<model::Index> sorted_to_original) noexcept
        : records_info_(records_info),
          column_matches_sim_info_(std::move(column_matches_sim_info)),
          column_matches_lhs_ids_info_(std::move(column_matches_lhs_ids_info)),
          sorted_to_original_(std::move(sorted_to_original)) {}

    static std::pair<SimilarityData, std::vector<bool>> CreateFrom(
            indexes::RecordsInfo* records_info, ColMatchesInfo column_matches_info);

    [[nodiscard]] std::size_t GetColumnMatchNumber() const noexcept {
        return column_matches_sim_info_.size();
    }

    [[nodiscard]] lattice::Rhs CreateMaxRhs() const noexcept {
        lattice::Rhs max_rhs;
        max_rhs.reserve(GetColumnMatchNumber());
        for (ColumnMatchInfo const& cm_info : column_matches_sim_info_) {
            max_rhs.push_back(cm_info.similarity_info.classifier_values.size() - 1);
        }
        return max_rhs;
    }

    std::vector<LhsCCVIdsInfo> const& GetLhsIdsInfo() const noexcept {
        return column_matches_lhs_ids_info_;
    }

    [[nodiscard]] std::vector<ColumnMatchInfo> const& GetColumnMatchesInfo() const noexcept {
        return column_matches_sim_info_;
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetColMatchIndices(
            model::Index index) const {
        auto const& [_, left_column_index, right_column_index] = column_matches_sim_info_[index];
        return {left_column_index, right_column_index};
    }

    [[nodiscard]] std::vector<model::Index> const& GetIndexMapping() const noexcept {
        return sorted_to_original_;
    }

    [[nodiscard]] model::md::DecisionBoundary GetLhsDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;

    [[nodiscard]] model::md::DecisionBoundary GetDecisionBoundary(
            model::Index column_match_index,
            ColumnClassifierValueId classifier_value_id) const noexcept;
};

}  // namespace algos::hymd
