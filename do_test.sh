#!/bin/bash

#current_date_time="`date +%Y_%m_%d_%H:%M:%S`"
#echo $current_date_time
#path="test_results_$current_date_time"
path="test_results"
num_threads=0

for i in "$@"
    do
    case $i in
    -t*|--threads=*)
        num_threads=${i#-t}
        num_threads=${num_threads#--threads=}
        ;;
    -p*|--path=*)
        path=${i#-p}
        path=${path#--path=}
    esac
done

#cpupower frequency-set -d 4.5GHz
#cpupower frequency-set -u 4.5GHz
for dataset in cora flights adult restaurants; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run ./build/target/input_data/$dataset.tsv $'\t' 0 $num_threads > $path/$dataset/log$i.txt
    done
done

#for dataset in notebook; do
#    echo "Started $dataset"
#    mkdir -p $path/$dataset
#    for i in {0..4}; do
#        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
#        ./build/target/Desbordante_run $dataset.csv ',' 1 $num_threads > $path/$dataset/log$i.txt
#    done
#done

for dataset in cddb; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run ./build/target/input_data/$dataset.tsv $'\t' 1 $num_threads > $path/$dataset/log$i.txt
    done
done
#cpupower frequency-set -d 0.8GHz
#cpupower frequency-set -u 4.5GHz
