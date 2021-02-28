print('Python module loaded')

import numpy as np

def sum(i, j):
    return np.array(i) + np.array(j).tolist() # using numpy arrays as return types would require eigen