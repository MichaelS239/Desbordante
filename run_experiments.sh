#!/bin/bash

begin_commit=$(git rev-parse --short HEAD)
end_commit=$(git log --oneline --grep="Add main testing script" --pretty=format:"%h")
commits_to_revert=$(git log HEAD...$end_commit --pretty=format:"%h")
for commit in $commits_to_revert
    do
    git revert --no-edit $commit
done
./build.sh -n -j$(nproc)
git reset --hard $begin_commit
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_performance/optimized
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/columns --path=./test_performance/columns
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/rows --path=./test_performance/rows
git checkout hymd-experiments-original
./build.sh -n -j$(nproc)
./do_test.sh --mode=performance -v --dataset_path=./test_datasets/performance --path=./test_performance/original
#git checkout hymd-experiments2
#./build.sh -n -j$(nproc)
#for i in {1..$(nproc)}; do
#    ./do_test.sh --mode=performance -v -t$i --path=./test_performance/thread$i
#done
#./do_test.sh --mode=performance -v -l --path=./test_performance/no_levels
#./do_test.sh --mode=performance -v -n --path=./test_performance/no_delete_empty_nodes
#./build.sh -n -j$(nproc)
#./do_test.sh --mode=stats -v --path=./test_stats
#./build.sh -n -p -j$(nproc)
#./do_test.sh --mode=python -v --path=./test_python
