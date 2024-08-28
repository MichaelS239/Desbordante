#!/bin/bash

git checkout hymd-experiments2
./build.sh -n -j$(nproc)
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_performance/optimized
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/columns --path=./test_performance/columns
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/rows --path=./test_performance/rows
#for i in {1..$(nproc)}; do
#    ./do_test.sh --mode=performance -v -t$i --path=./test_performance/thread$i
#done
#./do_test.sh --mode=performance -v -l --path=./test_performance/no_levels
#./do_test.sh --mode=performance -v -n --path=./test_performance/no_delete_empty_nodes
git checkout hymd-experiments-original
./build.sh -n -j$(nproc)
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_performance/original
#git checkout hymd-experiments2-stats
#./build.sh -n -j$(nproc)
#./do_test.sh --mode=stats -v --dataset_path=./test_datasets/stats --path=./test_stats_optimized
#./do_test.sh --mode=stats -v -l --dataset_path=./test_datasets/stats --path=./test_stats_no_levels
#./do_test.sh --mode=stats -v -n --dataset_path=./test_datasets/stats --path=./test_stats_no_delete_empty_nodes
#./build.sh -n -p -j$(nproc)
#./do_test.sh --mode=python -v --dataset_path=./test_datasets/stats --path=./test_python
