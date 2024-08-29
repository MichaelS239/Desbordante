#!/bin/bash

#current_date_time="`date +%Y_%m_%d_%H:%M:%S`"
#echo $current_date_time
#path="test_results_$current_date_time"
path="test_results"
dataset_path="test_datasets"
mode="stats"

for i in "$@"
    do
    case $i in
    -p*|--path=*)
        path=${i#-p}
        path=${path#--path=}
        ;;
    -d*|--dataset_path=*)
        dataset_path=${i#-p}
        dataset_path=${dataset_path#--dataset_path=}
        ;;
    -m*|--mode=*)
        mode=${i#-m}
        mode=${mode#--mode=}
    esac
done

run_number=0
if [[ $mode == "performance" ]]; then
    run_number=5
elif [[ $mode == "stats" ]]; then
    run_number=3
else
    run_number=1
fi

IFS=" " # pass tabs to commands

#cpupower frequency-set -d 4.5GHz
#cpupower frequency-set -u 4.5GHz
while read -r dataset_info; do
    dataset=${dataset_info%%$" "*}
    dataset_info=$(echo -e $dataset_info)
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for ((i=1;i<=run_number;i++)); do
        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
        /usr/bin/time -v -o $path/$dataset/log$i.txt -a ./build/target/Desbordante_run $dataset_path/$dataset_info > $path/$dataset/log$i.txt
    done
done < $dataset_path/datasets_info.txt
#cpupower frequency-set -d 0.8GHz
#cpupower frequency-set -u 4.5GHz

unset IFS
