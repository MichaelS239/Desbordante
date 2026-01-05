#!/bin/bash

#current_date_time="`date +%Y_%m_%d_%H:%M:%S`"
#echo $current_date_time
#path="test_results_$current_date_time"
path="test_results"
dataset_path="test_datasets"
threshold_path="test_thresholds"
dif_table_path="test_dif_tables"
mode="performance"

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

run_number=5

IFS=" " # pass tabs to commands

swapoff -a
cpupower frequency-set -d 4.1GHz
cpupower frequency-set -u 4.1GHz
for dataset in $dataset_path/*; do
    dataset_name=${dataset#"$dataset_path/"}
    thresholds_name="$threshold_path/${dataset_name%".csv"}_thresholds.txt"
    dif_table_name="$dif_table_path/${dataset_name%".csv"}_dif_table.csv"
    echo "Started $dataset_name"
    #echo $thresholds_name
    #echo $dif_table_name
    mkdir -p $path/$dataset_name
    for ((i=1;i<=run_number;i++)); do
        echo "Run $i"
        echo 3 > /proc/sys/vm/drop_caches
        if [[ $mode == "java" ]]; then
            /usr/bin/time -v -o $path/$dataset_name/log$i.txt -a java -jar FastDD-1.0-SNAPSHOT-jar-with-dependencies.jar $dataset -1 $thresholds_name &> $path/$dataset_name/log$i.txt
        else
            /usr/bin/time -v -o $path/$dataset_name/log$i.txt -a ./build/target/Desbordante_run $dataset $dif_table_name &> $path/$dataset_name/log$i.txt
        fi
    done
done
cpupower frequency-set -d 0.8GHz
cpupower frequency-set -u 4.1GHz
swapon -a

unset IFS
