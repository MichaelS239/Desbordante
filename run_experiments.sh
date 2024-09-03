#!/bin/bash

./build.sh -n -j$(nproc)
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_results/test_performance/optimized
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/columns --path=./test_results/test_performance/columns
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/rows --path=./test_results/test_performance/rows
./do_test.sh --mode=metanome -v --dataset_path=./test_datasets/performance --path=./test_results/test_performance/metanome
./do_test.sh --mode=metanome -v --dataset_path=./test_datasets/columns --path=./test_results/test_performance/metanome_columns
./do_test.sh --mode=metanome -v --dataset_path=./test_datasets/rows --path=./test_results/test_performance/metanome_rows
#for i in {1..$(nproc)}; do
#    ./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance -t$i --path=./test_results/test_performance/thread$i
#done
#./do_test.sh --mode=performance -v -l --dataset_path=./test_datasets/performance --path=./test_results/test_performance/no_levels
#./do_test.sh --mode=performance -v -n --dataset_path=./test_datasets/performance --path=./test_results/test_performance/no_delete_empty_nodes
git checkout hymd-experiments-original
./build.sh -n -j$(nproc)
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_results/test_performance/original
#git checkout hymd-experiments2-stats2
#./build.sh -n -j$(nproc)
#./do_test.sh --mode=stats -v --dataset_path=./test_datasets/stats --path=./test_results/test_stats_optimized
#./do_test.sh --mode=stats -v -l --dataset_path=./test_datasets/stats --path=./test_results/test_stats_no_levels
#./do_test.sh --mode=stats -v -n --dataset_path=./test_datasets/stats --path=./test_results/test_stats_no_delete_empty_nodes
#./build.sh -n -p -j$(nproc)
#./do_test.sh --mode=python -v --dataset_path=./test_datasets/stats --path=./test_results/test_python
