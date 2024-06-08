#pragma once

namespace config::names {
constexpr auto kTable = "table";
constexpr auto kTables = "tables";
constexpr auto kCsvConfig = "csv_config";
constexpr auto kCsvConfigs = "csv_configs";
constexpr auto kCsvPath = "csv_path";
constexpr auto kCsvPaths = "csv_paths";
constexpr auto kSeparator = "separator";
constexpr auto kHasHeader = "has_header";
constexpr auto kEqualNulls = "is_null_equal_null";
constexpr auto kThreads = "threads";
constexpr auto kError = "error";
constexpr auto kErrorMeasure = "error_measure";
constexpr auto kMaximumLhs = "max_lhs";
constexpr auto kMaximumArity = "max_arity";
constexpr auto kSeed = "seed";
constexpr auto kMinimumSupport = "minsup";
constexpr auto kMinimumConfidence = "minconf";
constexpr auto kInputFormat = "input_format";
constexpr auto kTIdColumnIndex = "tid_column_index";
constexpr auto kItemColumnIndex = "item_column_index";
constexpr auto kFirstColumnTId = "has_tid";
constexpr auto kMetric = "metric";
constexpr auto kLhsIndices = "lhs_indices";
constexpr auto kRhsIndices = "rhs_indices";
constexpr auto kRhsIndex = "rhs_index";
constexpr auto kUCCIndices = "ucc_indices";
constexpr auto kParameter = "parameter";
constexpr auto kDistFromNullIsInfinity = "dist_from_null_is_infinity";
constexpr auto kQGramLength = "q";
constexpr auto kMetricAlgorithm = "metric_algorithm";
constexpr auto kRadius = "radius";
constexpr auto kRatio = "ratio";
constexpr auto kBinaryOperation = "bin_operation";
constexpr auto kFuzziness = "fuzziness";
constexpr auto kFuzzinessProbability = "p_fuzz";
constexpr auto kWeight = "weight";
constexpr auto kBumpsLimit = "bumps_limit";
constexpr auto kTimeLimitSeconds = "time_limit";
constexpr auto kIterationsLimit = "iterations_limit";
constexpr auto kACSeed = "ac_seed";
constexpr auto kPreciseAlgorithm = "precise_algorithm";
constexpr auto kApproximateAlgorithm = "approximate_algorithm";
constexpr auto kCfdMinimumSupport = "cfd_minsup";
constexpr auto kCfdMinimumConfidence = "cfd_minconf";
constexpr auto kCfdColumnsNumber = "columns_number";
constexpr auto kCfdTuplesNumber = "tuples_number";
constexpr auto kCfdMaximumLhs = "cfd_max_lhs";
constexpr auto kCfdSubstrategy = "cfd_substrategy";
constexpr auto kHllAccuracy = "hll_accuracy";
constexpr auto kSampleSize = "sample_size";
constexpr auto kIgnoreNullCols = "ignore_null_cols";
constexpr auto kIgnoreConstantCols = "ignore_constant_cols";
constexpr auto kGraphData = "graph";
constexpr auto kGfdData = "gfd";
constexpr auto kMemLimitMB = "mem_limit";
constexpr auto kDifferenceTable = "difference_table";
constexpr auto kNumRows = "num_rows";
constexpr auto kNumColumns = "num_columns";
constexpr auto kInsertStatements = "insert";
constexpr auto kDeleteStatements = "delete";
constexpr auto kUpdateStatements = "update";
constexpr auto kLeftTable = "left_table";
constexpr auto kRightTable = "right_table";
constexpr auto kPruneNonDisjoint = "prune_nondisjoint";
constexpr auto kMinSupport = "min_support";
constexpr auto kColumnMatches = "column_matches";
constexpr auto kMaxCardinality = "max_cardinality";
}  // namespace config::names
