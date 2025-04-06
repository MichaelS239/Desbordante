#include "differential_function_builder.h"

#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/regex.hpp>

#include "model/types/builtin.h"

namespace algos::dd {

std::pair<std::vector<double>, std::vector<double>> DifferentialFunctionBuilder::GetThresholds(
        model::ColumnIndex const column_index) {
    model::TypedColumnData const& dif_column =
            difference_typed_relation_->GetColumnData(column_index);

    std::size_t dif_num_rows = difference_typed_relation_->GetNumRows();

    boost::regex df_regex(R"(([>=|<=]) (.*)$)");
    boost::regex double_regex(
            R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
            R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
            R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)");

    std::vector<double> less_thresholds;
    std::vector<double> greater_thresholds;

    std::set<double> less_thresholds_set;
    std::set<double> greater_thresholds_set;

    for (std::size_t row_index = 0; row_index < dif_num_rows; row_index++) {
        model::TypeId type_id = dif_column.GetValueTypeId(row_index);
        if (type_id == +model::TypeId::kString) {
            std::string df_str = dif_column.GetDataAsString(row_index);
            boost::smatch matches;
            if (boost::regex_match(df_str, matches, df_regex)) {
                if (boost::regex_match(matches[2].str(), double_regex)) {
                    double const threshold =
                            model::TypeConverter<double>::kConvert(matches[1].str());
                    if (matches[1].str() == "<=") {
                        less_thresholds_set.insert(threshold);
                    } else {
                        greater_thresholds_set.insert(threshold);
                    }
                }
            }
        }
    }

    less_thresholds.insert(less_thresholds.end(), less_thresholds_set.begin(),
                           less_thresholds_set.end());
    greater_thresholds.insert(greater_thresholds.end(), greater_thresholds_set.begin(),
                              greater_thresholds_set.end());
    return {std::move(less_thresholds), std::move(greater_thresholds)};
}

void DifferentialFunctionBuilder::BuildDFList(
        std::vector<model::TypedColumnData> const& column_data) {
    for (model::ColumnIndex column_index = 0; column_index != num_columns_; ++column_index) {
        auto const [less_thresholds, greater_thresholds] = GetThresholds(column_index);
    }
}

}  // namespace algos::dd
