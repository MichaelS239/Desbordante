#!/bin/bash

#current_date_time="`date +%Y_%m_%d_%H:%M:%S`"
#echo $current_date_time
#path="test_results_$current_date_time"
path="test_results"
dataset_path="test_datasets"
num_threads=0
verbose=false
no_levels=false
delete_empty_nodes=true
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
    -t*|--threads=*)
        num_threads=${i#-t}
        num_threads=${num_threads#--threads=}
        ;;
    -v|--verbose)
        verbose=true
        ;;
    -l|--no_levels)
        no_levels=true
        ;;
    -n|--no_delete_empty_nodes)
        delete_empty_nodes=false
        ;;
    -m*|--mode=*)
        mode=${i#-m}
        mode=${mode#--mode=}
    esac
done

if [[ $mode == "python" ]]; then
    cp do_test.py ./build/target/do_test.py
fi

options="$num_threads"

if [[ $verbose == true ]]; then
    options="$options -v"
fi

if [[ $no_levels == true ]]; then
    options="$options -l"
fi

if [[ $delete_empty_nodes == false ]] && [[ $mode != "python" ]]; then
    options="$options -n"
fi

run_number=0
if [[ $mode == "performance" ]] || [[ $mode == "metanome" ]]; then
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
    separator=${dataset_info%$" "*}
    separator=${separator##*$" "}
    dataset_info=$(echo -e $dataset_info)
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for ((i=1;i<=run_number;i++)); do
        echo "Run $i"
        #echo 3 > /proc/sys/vm/drop_caches
        if [[ $mode == "python" ]]; then
            /usr/bin/time -v -o $path/$dataset/log$i.txt -a python3 ./build/target/do_test.py $dataset_path/$dataset_info $options > $path/$dataset/log$i.txt
        elif [[ $mode == "metanome" ]]; then
            /usr/bin/time -v -o $path/$dataset/log$i.txt -a java -cp metanome-cli-1.2-SNAPSHOT.jar:HyMD-1.2-SNAPSHOT.jar de.metanome.cli.App --algorithm de.metanome.algorithms.hymd.HyMD --tables $dataset_path/$dataset --separator $separator --table-key RELATION > $path/$dataset/log$i.txt
        else
            /usr/bin/time -v -o $path/$dataset/log$i.txt -a ./build/target/Desbordante_run $dataset_path/$dataset_info $options > $path/$dataset/log$i.txt
        fi
    done
done < $dataset_path/datasets_info.txt
#cpupower frequency-set -d 0.8GHz
#cpupower frequency-set -u 4.5GHz

unset IFS
