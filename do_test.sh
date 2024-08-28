#!/bin/bash

#current_date_time="`date +%Y_%m_%d_%H:%M:%S`"
#echo $current_date_time
#path="test_results_$current_date_time"
path="test_results3"
num_threads=0
verbose=false
no_levels=false
delete_empty_nodes=true

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
        ;;
    -v|--verbose)
        verbose=true
        ;;
    -l|--no_levels)
        no_levels=true
        ;;
    -n|--no_delete_empty_nodes)
        delete_empty_nodes=false
    esac
done

options="$num_threads"

if [[ $verbose == true ]]; then
    options="$options -v"
fi

if [[ $no_levels == true ]]; then
    options="$options -l"
fi

if [[ $delete_empty_nodes == false ]]; then
    options="$options -n"
fi

#cpupower frequency-set -d 4.5GHz
#cpupower frequency-set -u 4.5GHz
for dataset in cora flights adult restaurants; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run ./build/target/input_data/$dataset.tsv $'\t' 0 $options> $path/$dataset/log$i.txt
    done
done

#for dataset in notebook; do
#    echo "Started $dataset"
#    mkdir -p $path/$dataset
#    for i in {0..4}; do
#        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
#        ./build/target/Desbordante_run $dataset.csv ',' 1 $options > $path/$dataset/log$i.txt
#    done
#done

#for dataset in cddb; do
#    echo "Started $dataset"
#    mkdir -p $path/$dataset
#    for i in {0..4}; do
#        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
#        ./build/target/Desbordante_run ./build/target/input_data/$dataset.tsv $'\t' 1 $options > $path/$dataset/log$i.txt
#    done
#done
#cpupower frequency-set -d 0.8GHz
#cpupower frequency-set -u 4.5GHz
