#pragma once

#include <list>
#include <memory>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/dd/dd.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/types/builtin.h"

namespace algos::dd {

class FastDD : public Algorithm {
private:
    config::InputTable input_table_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    unsigned shard_length_;

    std::vector<model::TypeId> type_ids_;

    config::InputTable operator_difference_table_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;

    void RegisterOptions();
    void SetLimits();
    void CheckTypes();
    void ParseDifferenceTable();

    void ResetState() final {}

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    FastDD();

    std::list<model::DDString> GetDDs() const;
};

}  // namespace algos::dd
