#pragma once

#include <list>
#include <memory>
#include <vector>

#include "algorithms/dd/fastdd/model/differential_function.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/table/typed_column_data.h"

namespace algos::dd {
class DifferentialFunctionBuilder {
private:
    std::list<DifferentialFunction> differential_functions_;
    std::list<model::ColumnIndex> int_cols_;
    std::list<model::ColumnIndex> str_cols_;
    std::list<model::ColumnIndex> double_cols_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;
    model::ColumnIndex num_columns_;

    std::pair<std::vector<double>, std::vector<double>> GetThresholds(
            model::ColumnIndex const column_index);

public:
    DifferentialFunctionBuilder(std::shared_ptr<model::ColumnLayoutTypedRelationData> relation,
                                model::ColumnIndex num_columns)
        : difference_typed_relation_(relation), num_columns_(num_columns) {}

    void BuildDFList(std::vector<model::TypedColumnData> const& column_data);
};

}  // namespace algos::dd
