#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/dd/fastdd/model/differential_dependency.h"
#include "algorithms/dd/fastdd/model/match_df.h"
#include "algorithms/dd/fastdd/util/differential_function_builder.h"

namespace algos::dd {

class HybridEvidenceInverter {
private:
    std::vector<MatchDF> match_dfs_;
    std::vector<std::vector<DifferentialFunction>> dif_funcs_;
    std::vector<boost::dynamic_bitset<>> column_to_dif_funcs_;
    std::vector<std::size_t> dif_func_sizes_;
    std::vector<std::size_t> dif_func_nums_;
    std::vector<std::size_t> dif_func_to_node_id;
    std::size_t dif_func_num_;

    std::vector<boost::dynamic_bitset<>> dif_func_to_satisfied_bitsets_;
    std::vector<boost::dynamic_bitset<>> dif_func_to_not_satisfied_bitsets_;

    void BuildClueIndices();
    std::vector<DifferentialDependency> Minimize(std::vector<boost::dynamic_bitset<>> covers);

public:
    HybridEvidenceInverter(std::vector<MatchDF> match_dfs,
                           DifferentialFunctionBuilder const& df_builder);

    std::vector<DifferentialDependency> BuildDDs();
};

}  // namespace algos::dd
