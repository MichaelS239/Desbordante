#pragma once

#include <cstddef>
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

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;

    std::pair<std::vector<double>, std::vector<double>> GetThresholds(
            model::TypedColumnData const& dif_column, model::ColumnIndex const column_index) const;

    double CalculateDistance(model::ColumnIndex column_index,
                             std::pair<std::size_t, std::size_t> tuple_pair) const;

    std::pair<std::vector<double>, std::vector<double>> SampleThresholds(
            model::ColumnIndex const column_index, std::vector<std::size_t> const& row_nums,
            std::size_t row_limit, std::size_t threshold_num, double freq_boundary,
            double index_boundary) const;
    std::vector<std::size_t> SampleRows(std::size_t row_limit) const;

    void AddThresholds(std::vector<double> const& less_thresholds,
                       std::vector<double> const& greater_thresholds,
                       model::ColumnIndex const column_index);

public:
    DifferentialFunctionBuilder(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation, unsigned num_rows,
            model::ColumnIndex num_columns)
        : typed_relation_(typed_relation), num_rows_(num_rows), num_columns_(num_columns) {}

    void BuildDFList(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation);
};

}  // namespace algos::dd
