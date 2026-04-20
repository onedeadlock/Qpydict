#ifndef QPy_TYPES_H
#define QPy_TYPES_H

#ifndef Py_SSIZE_T_CLEAN
#define Py_SSIZE_T_CLEAN
#endif
#include <Python.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define QPy_UNUSED(x)    Py_UNUSED(x)
#define QPy_T_SSIZE      Py_T_PYSSIZET
#define QPy_ARRAY_SIZE   sizeof(QPyDict_Array_)
#define QPy_CACHE_SIZE   sizeof(QPyDict_Cache_)
#define QPy_DEFAULT_SIZE 0

typedef PyObject * QPyDict_PyObject;
typedef Py_ssize_t QPy_ssize_t;
typedef Py_hash_t  QPy_hash_t;

// Internal Types
typedef struct {
    uint8_t cache;
} QPyDict_Cache_;

typedef struct {
    QPyDict_PyObject key;
    QPyDict_PyObject value;
    QPy_hash_t       hash;
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
