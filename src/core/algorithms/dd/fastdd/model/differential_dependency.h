#pragma once

#include <vector>

#include "algorithms/dd/fastdd/model/differential_function.h"

namespace algos::dd {

class DifferentialDependency {
    std::vector<DifferentialFunction> lhs_;
    DifferentialFunction rhs_;
};

}  // namespace algos::dd
