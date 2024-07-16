#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"

#include <atomic>
#include <cstddef>
#include <numeric>
#include <set>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/build_indexes.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/encode_results.h"
#include "algorithms/md/hymd/preprocessing/valid_table_results.h"
#include "algorithms/md/hymd/utility/make_unique_for_overwrite.h"
#include "config/exceptions.h"
#include "model/types/double_type.h"
#include "model/types/string_type.h"

namespace {
using namespace algos::hymd;
using SimValPair = std::pair<preprocessing::Similarity, ValueIdentifier>;

/* An optimized version of the Levenshtein distance computation algorithm from
 * https://en.wikipedia.org/wiki/Levenshtein_distance, using preallocated buffers
 */
unsigned LevenshteinDistance(auto const* l_ptr, auto const* r_ptr, unsigned* v0, unsigned* v1,
                             unsigned max_dist) noexcept {
    std::size_t r_size = r_ptr->size();
    assert(v0 < v1);
    std::size_t l_size = l_ptr->size();
    if (r_size > l_size) {
        std::swap(l_ptr, r_ptr);
        std::swap(l_size, r_size);
    }
    {
        std::size_t const len_diff = l_size - r_size;
        if (len_diff > max_dist) {
            return len_diff;
        }
    }

    auto const& l = *l_ptr;
    auto const& r = *r_ptr;

    std::iota(v0, v0 + r_size + 1, 0);

    auto compute_arrays = [&](auto* v0, auto* v1, unsigned i) {
        *v1 = i + 1;
        auto const li = l[i];

        for (unsigned j = 0; j != r_size;) {
            unsigned const insert_cost = v1[j] + 1;
            unsigned const substition_cost = v0[j] + (li != r[j]);
            ++j;
            unsigned const deletion_cost = v0[j] + 1;

            v1[j] = std::min({deletion_cost, insert_cost, substition_cost});
        }
    };
    auto loop_to_l_size = [&l_size, &v0, &v1, &compute_arrays]() {
        for (unsigned i = 0; i != l_size; ++i) {
            compute_arrays(v0, v1, i);
            ++i;
            compute_arrays(v1, v0, i);
        }
    };
    if (l_size & 1) {
        --l_size;
        loop_to_l_size();
        compute_arrays(v0, v1, l_size);
        return v1[r_size];
    } else {
        loop_to_l_size();
        return v0[r_size];
    }
}

using namespace algos::hymd::preprocessing;

std::size_t GetLargestStringSize(DataInfo const& data_info_left) {
    std::size_t const element_number = data_info_left.GetElementNumber();
    std::size_t max_size = 0;
    for (model::Index i = 0; i != element_number; ++i) {
        auto const& left_string = model::Type::GetValue<std::string>(data_info_left.GetAt(i));
        std::size_t const left_size = left_string.size();
        if (left_size > max_size) max_size = left_size;
    }
    return max_size;
}

struct ValueProcessingWorker {
    std::shared_ptr<DataInfo const> const& data_info_left;
    std::shared_ptr<DataInfo const> const& data_info_right;
    std::vector<indexes::PliCluster> const& clusters_right;
    Similarity const min_sim;
    ValidTableResults<Similarity>& task_data;
    std::size_t const data_left_size = data_info_left->GetElementNumber();
    std::size_t const data_right_size = data_info_right->GetElementNumber();
    std::size_t const largest_string_size = GetLargestStringSize(*data_info_left);
    std::atomic<bool> dissimilar_found = false;
    std::atomic<model::Index> current_index = 0;

    void AddValue(RowInfo<Similarity>& row_info, ValueIdentifier value_id, Similarity sim) {
        auto& [sim_value_id_vec, valid_records_number] = row_info;
        sim_value_id_vec.emplace_back(sim, value_id);
        valid_records_number += clusters_right[value_id].size();
    }

    void Start(auto method) {
        auto buf = /* TODO: replace with std::make_unique_for_overwrite when GCC in CI is
                      upgraded */
                utility::MakeUniqueForOverwrite<unsigned[]>((largest_string_size + 1) * 2);
        ValueIdentifier left_value_id;
        auto buf1 = buf.get();
        auto buf2 = buf1 + largest_string_size + 1;
        bool found_dissimilar = false;
        while ((left_value_id = current_index++) < data_left_size) {
            bool found_dissimilar_here = (this->*method)(left_value_id, buf1, buf2);
            if (found_dissimilar_here) found_dissimilar = true;
        }
        current_index = data_left_size;
        if (found_dissimilar) dissimilar_found.store(true, std::memory_order::release);
    }

    void StartFull() {
        Start(&ValueProcessingWorker::ProcessFull);
    }

    bool ProcessFull(ValueIdentifier const left_value_id, unsigned* buf1, unsigned* buf2) {
        return CalcAndAdd(left_value_id, buf1, buf2, task_data[left_value_id], 0);
    }

    void StartSame() {
        Start(&ValueProcessingWorker::ProcessSame);
    }

    bool ProcessSame(ValueIdentifier const left_value_id, unsigned* buf1, unsigned* buf2) {
        RowInfo<Similarity>& row_info = task_data[left_value_id];
        AddValue(row_info, left_value_id, 1.0);
        return CalcAndAdd(left_value_id, buf1, buf2, row_info, left_value_id + 1);
    }

    bool CalcAndAdd(ValueIdentifier left_value_id, unsigned* buf1, unsigned* buf2,
                    RowInfo<Similarity>& row_info, ValueIdentifier start_from) {
        // Ignore nulls and empty for now.
        auto const& left_string =
                model::Type::GetValue<std::string>(data_info_left->GetAt(left_value_id));
        std::size_t const left_size = left_string.size();
        bool dissimilar_found_here = false;
        for (ValueIdentifier value_id_right = start_from; value_id_right != data_right_size;
             ++value_id_right) {
            auto const& right_string =
                    model::Type::GetValue<std::string>(data_info_right->GetAt(value_id_right));
            std::size_t const max_dist = std::max(left_size, right_string.size());
            // Left has to be second since that's what the function uses to determine the buffer
            // size it needs
            Similarity similarity = 1.0;
            if (max_dist != 0) {
                unsigned lim = max_dist * (1 - min_sim);
                unsigned dist = LevenshteinDistance(&right_string, &left_string, buf1, buf2, lim);
                similarity = (max_dist - dist) / static_cast<Similarity>(max_dist);
                // Don't store 0.0 no matter the value of min_sim.
                if (similarity < min_sim) similarity = kLowestBound;
            }
            if (similarity == kLowestBound) {
                dissimilar_found_here = true;
                continue;
            }
            AddValue(row_info, value_id_right, similarity);
        }
        return dissimilar_found_here;
    }

    bool DissimilarFound() const noexcept {
        return dissimilar_found.load(std::memory_order::acquire);
    }
};
}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {

indexes::SimilarityMeasureOutput LevenshteinSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right,
        util::WorkerThreadPool& thread_pool) const {
    ValidTableResults<Similarity> task_data{data_info_left->GetElementNumber()};
    ValueProcessingWorker worker{data_info_left, data_info_right, clusters_right, min_sim_,
                                 task_data};
    if (data_info_left == data_info_right) {
        thread_pool.SetWork([&worker]() { worker.StartSame(); });
    } else {
        thread_pool.SetWork([&worker]() { worker.StartFull(); });
    }
    thread_pool.WorkUntilComplete();

    auto additional_bounds = {1.0, kLowestBound};
    std::span additional_results(additional_bounds.begin(), worker.DissimilarFound() ? 2 : 1);
    auto [similarities, enumerated_results] =
            EncodeResults(std::move(worker.task_data), additional_results);
    if (data_info_left == data_info_right) SymmetricClosure(enumerated_results, clusters_right);

    auto pick_index_uniform = [this](auto const& bounds) {
        return ccv_id_pickers::IndexUniform(bounds.size(), size_limit_);
    };
    return BuildIndexes(std::move(enumerated_results), std::move(similarities), clusters_right,
                        pick_index_uniform);
}

LevenshteinSimilarityMeasure::LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim,
                                                           std::size_t size_limit)
    : SimilarityMeasure(std::make_unique<model::StringType>(),
                        std::make_unique<model::DoubleType>()),
      min_sim_(min_sim),
      size_limit_(size_limit) {}

LevenshteinSimilarityMeasure::Creator::Creator(model::md::DecisionBoundary min_sim, std::size_t size_limit)
    : SimilarityMeasureCreator(kName),
      min_sim_(min_sim),
      size_limit_(size_limit) {
    if (!(0.0 <= min_sim_ && min_sim_ <= 1.0))
        throw config::ConfigurationError("Minimum similarity out of range");
}

}  // namespace algos::hymd::preprocessing::similarity_measure
