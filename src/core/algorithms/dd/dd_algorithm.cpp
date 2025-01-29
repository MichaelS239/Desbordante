#include "algorithms/dd/dd_algorithm.h"

#include "config/tabular_data/input_table/option.h"

namespace algos::dd {

DDAlgorithm::DDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOption(config::kTableOpt(&input_table_));
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

}  // namespace algos::dd
