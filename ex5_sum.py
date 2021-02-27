try:
    import numpy as np
except:
    print('numpy is missing. Install it with \'pip install numpy\'')

def sum(i, j):
    return np.sum(np.array(i) + np.array(j))