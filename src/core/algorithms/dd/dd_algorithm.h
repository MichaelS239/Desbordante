#pragma once

#include <list>
#include <string_view>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/dd/dd.h"
#include "config/tabular_data/input_table_type.h"
#include "util/primitive_collection.h"

namespace algos::dd {

class DDAlgorithm : public Algorithm {
private:
    void ResetState() final {
        dd_collection_.Clear();
        ResetDDAlgorithmState();
    }

    virtual void ResetDDAlgorithmState() = 0;

protected:
    config::InputTable input_table_;

    util::PrimitiveCollection<model::DDString> dd_collection_;

    void RegisterDD(model::DDString dd_to_register) {
        dd_collection_.Register(std::move(dd_to_register));
    }

    explicit DDAlgorithm(std::vector<std::string_view> phase_names);

public:
    std::list<model::DDString> const& DDList() const noexcept {
        return dd_collection_.AsList();
    }
};

}  // namespace algos::dd
