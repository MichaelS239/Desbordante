#include "algorithms/dd/fastdd/fastdd.h"

#include <chrono>
#include <cstddef>
#include <stdexcept>

#include <easylogging++.h>

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/column_index.h"

namespace algos::dd {

FastDD::FastDD() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void FastDD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&operator_difference_table_, kOperatorDifferenceTable,
                          kDOperatorDifferenceTable, default_table});
    RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
    RegisterOption(Option{&num_columns_, kNumColumns, kDNUmColumns, 0U});
}

void FastDD::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kOperatorDifferenceTable, kNumRows, kNumColumns});
}

void FastDD::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                       false);  // nulls are ignored
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: DD mining is meaningless.");
    }
}

void FastDD::SetLimits() {
    unsigned all_rows_num = typed_relation_->GetNumRows();
    model::ColumnIndex all_columns_num = typed_relation_->GetNumColumns();
    if (num_rows_ > all_rows_num) {
        throw std::invalid_argument(
                "'num_rows' must be less or equal to the number of rows in the table (total "
                "rows: " +
                std::to_string(all_rows_num) + ")");
    }
    if (num_columns_ > all_columns_num) {
        throw std::invalid_argument(
                "'num_columns' must be less or equal to the number of columns in the table (total "
                "columns: " +
                std::to_string(all_columns_num) + ")");
    }
    if (num_rows_ == 0) num_rows_ = all_rows_num;
    if (num_columns_ == 0) num_columns_ = all_columns_num;
}

void FastDD::CheckTypes() {
    type_ids_.resize(num_columns_, model::TypeId::kUndefined);
    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kUndefined) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" contains values of different types.");
        }

        type_ids_[column_index] = type_id;

        for (std::size_t row_index = 0; row_index < num_rows_; row_index++) {
            if (column.IsNull(row_index)) {
                throw std::runtime_error("Some of the value coordinates are nulls.");
            }
            if (column.IsEmpty(row_index)) {
                throw std::runtime_error("Some of the value coordinates are empty.");
            }
        }
    }
}

void FastDD::ParseDifferenceTable() {
    if (operator_difference_table_) {
        difference_typed_relation_ =
                model::ColumnLayoutTypedRelationData::CreateFrom(*operator_difference_table_,
                                                                 false);  // nulls are ignored
        if (typed_relation_->GetNumColumns() != num_columns_) {
            throw std::invalid_argument(
                    "The number of columns in the difference table must be equal to the number of "
                    "columns in the loaded table or to 'num_columns' if specified");
        }
    }
}

unsigned long long FastDD::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    LOG(DEBUG) << "Start";

    SetLimits();
    CheckTypes();
    ParseDifferenceTable();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(DEBUG) << "Algorithm time: " << elapsed_milliseconds.count();
    return elapsed_milliseconds.count();
}

std::list<model::DDString> FastDD::GetDDs() const {
    return {};
}

}  // namespace algos::dd
