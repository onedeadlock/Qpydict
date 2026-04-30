#!/bin/python
import time
from shared_lib.Qpydict import qpydict

# instantiate class
tp = [(x, y) for x in range(10) for y in range(10, 20)]

print(len(tp))

k = time.time()

cls = qpydict(tp)

print(time.time() - k);

print(cls._capacity)

