import datetime
import statistics
import os
import sys

D_LEN = len('2024-01-02 14:53:14,039')
path=sys.argv[1]


def get_time(log_line):
    return datetime.datetime.strptime(log_line[:D_LEN], '%Y-%m-%d %H:%M:%S,%f')


def get_run_times(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
    start_time = get_time(lines[0])
    sim_done_time = get_time(next(line for line in lines if line.endswith('Finished similarity calculation\n')))
    done_time = get_time(next(line for line in lines if line.endswith('Done!\n')))
    return start_time, sim_done_time, done_time


def print_ds_stats(dataset_filename):
    times = [get_run_times(f'{dataset_filename}/log{i}.txt') for i in range(5)]
    total_times = [(done - sim_done) / datetime.timedelta(milliseconds=1) for start, sim_done, done in times]
    print(dataset_filename)
    print(f'{min(total_times)=}')
    print(f'{max(total_times)=}')
    print(f'{statistics.mean(total_times)=}')


for ds in os.listdir(path):
    print()
    print_ds_stats(f'{path}/{ds}')
