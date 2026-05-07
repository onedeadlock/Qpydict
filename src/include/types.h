#ifndef QPy_TYPES_H
#define QPy_TYPES_H

#ifndef Py_SSIZE_T_CLEAN
#define Py_SSIZE_T_CLEAN
#endif
#include <Python.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include "mm.h"

#ifndef QPy_MM_UNSUPPROTED
#   ifdef QPy_MM_SSE
#       if QPy_NGROUP == 32
            typedef __m256i mm_t; 
#       else
            typedef __m128i mm_t;
#       endif
#   elif QPy_MM_NEON
#       ifdef QPy_NGROUP == 16
            typedef uint8x16_t mm_t;
#       else
            typedef uint8x8_t mm_t;
#       endif
#   else
        typedef uint64_t mm_t;
#   endif
#endif

#define QPy_UNUSED(x)    Py_UNUSED(x)
#define QPy_T_SSIZE      Py_T_PYSSIZET
#define QPy_ARRAY_SIZE   sizeof(QPyDict_Array_)
#define QPy_CACHE_SIZE   sizeof(QPyDict_Cache_)
#define QPy_DEFAULT_SIZE 0 // TODO: set

typedef PyObject * QPy_PyObject;
typedef Py_ssize_t QPy_ssize_t;
typedef Py_hash_t  QPy_hash_t;

// Internal Types
typedef struct {
    uint8_t cache[1];
} QPyDict_Cache_;

typedef struct {
    QPy_PyObject key;
    QPy_PyObject value;
    QPy_hash_t   hash;
} QPyDict_Array_;

typedef QPyDict_Array_ * QPyDict_Array;
typedef QPyDict_Cache_ * QPyDict_Cache;

// Instance Object
typedef struct {
    PyObject_HEAD
    QPyDict_Cache  cache;
    QPyDict_Array  entries;
    QPy_ssize_t    nentries;
    QPy_ssize_t    used_entries;
    QPy_ssize_t    group_size;
} QPyDictObject;

#endif // QPy_TYPES_H
