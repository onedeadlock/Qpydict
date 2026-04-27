#!/bin/python
import time
from shared_lib.Qpydict import qpydict

# instantiate class
tp = [(x, y) for x in range(1000) for y in range(1000)]
print(len(tp))

k = time.time()
cls = qpydict(tp, a=3, b=5, k=6, p=6)
#print(cls._capacity)
print(time.time() - k);

print(cls._capacity)

