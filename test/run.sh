#!/bin/bash

# compile to object files
gcc -fno-strict-overflow -Wsign-compare -O2 -Wall -fPIC -I../src -I/root/include -I/usr/include/python3.13 -c ../src/internal/methods.c -o ./build_temp/../src/internal/methods.o
gcc -fno-strict-overflow -Wsign-compare -O2 -Wall -fPIC -I../src -I/root/include -I/usr/include/python3.13 -c ../src/module.c -o ./build_temp/../src/module.o

# linking 
gcc -shared -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-Bsymbolic-functions -Wl,-z,relro -g -fwrapv -O2 ./build_temp/../src/internal/methods.o ./build_temp/../src/module.o -L/usr/lib/arm-linux-gnueabihf -o ./shared_lib/Qpydict.abi3.so
