#include "algorithms/dd/fastdd/model/pli_shard.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <easylogging++.h>

#include "algorithms/dd/fastdd/providers/index_provider.h"
#include "model/table/column_index.h"
#include "model/types/builtin.h"
#include "model/types/type.h"

namespace algos::dd {

Pli::Pli(std::vector<Cluster> raw_clusters, std::vector<std::size_t> keys,
         std::unordered_map<std::size_t, std::size_t> translator)
    : clusters_(std::move(raw_clusters)),
      keys_(std::move(keys)),
      key_to_cluster_id_map_(std::move(translator)) {}

bool Pli::TryGetClusterIdByKey(std::size_t key, std::size_t& cluster_id) const {
    auto it = key_to_cluster_id_map_.find(key);
    if (it != key_to_cluster_id_map_.end()) {
        cluster_id = it->second;
        return true;
    }
    return false;
}

std::size_t Pli::GetClusterIdByKey(std::size_t key) const {
    auto it = key_to_cluster_id_map_.find(key);
    if (it != key_to_cluster_id_map_.end()) {
        return it->second;
    }

    throw std::runtime_error("No cluster can be found by key " + std::to_string(key));
}

std::size_t Pli::GetFirstIndexWhereKeyIsLTE(std::size_t target, std::size_t l) const {
    auto it = std::lower_bound(keys_.begin() + l, keys_.end(), target, std::greater<std::size_t>());
    return std::distance(keys_.begin(), it);
}

std::string Pli::ToString() const {
    std::stringstream ss;

    ss << "\tKeys: [";
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << keys_[i];
    }
    ss << "]\n";
    ss << "\tClusters:\n";
    for (auto const& key : keys_) {
        auto id = GetClusterIdByKey(key);

        ss << "\t\tKey " << key << " -> Cluster #" << id << ": [";
        for (std::size_t j = 0; j < clusters_[id].size(); ++j) {
            if (j > 0) ss << ", ";
            ss << clusters_[id][j];
        }
        ss << "]\n";
    }
    return ss.str();
}

std::string PliShard::ToString() const {
    std::stringstream ss;
    ss << "Range: [" << beg_ << ", " << end_ << ")\n";
    for (std::size_t i = 0; i < plis_.size(); ++i) {
        ss << "PLI #" << (i + 1) << "\n";
        ss << plis_[i].ToString();
    }
    return ss.str();
}

std::vector<std::size_t> PliShardBuilder::ColumnToHash(model::TypedColumnData const& column,
                                                       model::ColumnIndex column_index) {
    std::vector<std::size_t> hashed_column(column.GetNumRows());
    for (std::size_t row = 0; row < column.GetNumRows(); ++row) {
        hashed_column[row] = providers_[column_index].GetIndex(column.GetValue(row));
    }

    return hashed_column;
}

/**
 * FastADC hashes the table with sorted hash, that is, the same values are substituited by
 * the same integers, and higher values are replaced by larger integers. So before
 * creating table with values as keys (hashes), we need to store all values in the
 * related three Providers and sort them.
 */
void PliShardBuilder::AddTableToHashAndSortProviders(
        std::vector<model::TypedColumnData> const& input) {
    providers_.reserve(input.size());
    for (model::ColumnIndex col = 0; col < input.size(); col++) {
        auto const& column = input[col];
        std::size_t rows = column.GetNumRows();
        model::TypeId type_id = column.GetTypeId();
        model::Type const& type = column.GetType();
        providers_.emplace_back(type.GetHasher(), type.GetEqual());
        for (std::size_t row = 0; row < rows; row++) {
            providers_[col].GetIndex(column.GetValue(row));
        }
        if (model::Type::IsDistanceOrdered(type_id)) {
            /*providers_[col].Sort([type_id](std::byte const* l, std::byte const* r) {
                auto type = model::CreateType(type_id, false);
                return type->Compare(l, r) == model::CompareResult::kLess;
            });*/
            providers_[col].Sort(type.GetComparator());
        }
    }
}

std::vector<PliShard> PliShardBuilder::BuildPliShards(
        std::vector<model::TypedColumnData> const& input) {
    model::ColumnIndex cols_num = input.size();
    assert(cols_num != 0);
    std::vector<std::vector<std::size_t>> hashed_input(cols_num, std::vector<std::size_t>());

    AddTableToHashAndSortProviders(input);

    for (model::ColumnIndex column_index = 0; column_index < cols_num; ++column_index) {
        hashed_input[column_index] = ColumnToHash(input[column_index], column_index);
    }

    std::size_t row_beg = 0, row_end = input[0].GetNumRows();
    std::size_t shards_num = (row_end - row_beg - 1) / shard_length_ + 1;

    std::vector<PliShard> pli_shards;
    pli_shards.reserve(shards_num);

    for (std::size_t i = 0; i < shards_num; i++) {
        std::size_t shard_beg = row_beg + i * shard_length_;
        std::size_t shard_end = std::min(row_end, shard_beg + shard_length_);
        std::vector<Pli> plis;
        plis.reserve(cols_num);

        for (model::ColumnIndex col = 0; col < cols_num; col++) {
            // model::Type t = input[col].GetType();
            if (!hashed_input[col].empty()) {
                Pli pli = BuildPli(hashed_input[col], input[col].IsNumeric(), shard_beg, shard_end);
                plis.push_back(std::move(pli));
            }
        }

        pli_shards.emplace_back(plis, shard_beg, shard_end);
    }

    return pli_shards;
}

Pli PliShardBuilder::BuildPli(std::vector<std::size_t> const& col_values, bool is_num,
                              std::size_t beg, std::size_t end) {
    std::unordered_set<std::size_t> unique_keys(col_values.begin() + beg, col_values.begin() + end);
    std::vector<std::size_t> keys(unique_keys.begin(), unique_keys.end());

    if (is_num) {
        std::sort(keys.begin(), keys.end(), std::greater<std::size_t>());
    }

    std::unordered_map<std::size_t, std::size_t> key_to_cluster_id;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        key_to_cluster_id[keys[i]] = i;
    }

    std::vector<Cluster> clusters(keys.size());
    for (std::size_t row = beg; row < end; ++row) {
        std::size_t key = col_values[row];
        std::size_t cluster_id = key_to_cluster_id[key];
        clusters[cluster_id].push_back(row);
    }

    return {clusters, keys, key_to_cluster_id};
}

}  // namespace algos::dd
