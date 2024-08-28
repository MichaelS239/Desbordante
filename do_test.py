import desbordante
import sys

if (len(sys.argv) < 5 or len(sys.argv) > 7):
    sys.exit()
table = sys.argv[1]
separator = sys.argv[2]
has_header = True if sys.argv[3] == '1' else False
num_threads = int(sys.argv[4])
verbose = False
no_levels = False
for i in range(5,len(sys.argv)):
    if (sys.argv[i] == "-v"):
        verbose = True
    elif (sys.argv[i] == "-l"):
        no_levels = True
level_def = "lattice" if no_levels else "cardinality"

algo = desbordante.md.algorithms.HyMD()
algo.load_data(left_table = (table, separator, has_header))
algo.execute(threads = num_threads,level_definition = level_def)
