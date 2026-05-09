import desbordante

TABLE = 'test_input_data/TestWide.csv'

algo = desbordante.fd.algorithms.DFD()
algo.load_data(table=(TABLE, ',', True))
algo.execute()
result = algo.get_fds()
print('FDs:')
for fd in result:
    print(fd)
