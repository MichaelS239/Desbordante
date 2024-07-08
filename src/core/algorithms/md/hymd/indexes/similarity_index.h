#pragma once

#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::indexes {
using RecSet = boost::unordered::unordered_flat_set<RecordIdentifier>;
using MatchingRecsMapping = boost::container::flat_map<ColumnClassifierValueId, RecSet>;
using SimilarityIndex = std::vector<MatchingRecsMapping>;
}  // namespace algos::hymd::indexes
