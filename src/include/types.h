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

#define QPy_T_SSIZE      Py_T_PYSSIZET
#define QPy_ARRAY_SIZE   sizeof(0)
#define QPy_CACHE_SIZE   sizeof(kvpair_t)
#define QPy_DEFAULT_SIZE 0 // TODO: set

typedef Py_ssize_t ssize_t;
typedef Py_hash_t  hash_t;

typedef struct {
    PyObject *key;
    hash_t   hash;
} khpair_t;

typedef struct {
    struct 
    {
	    uint8_t  *cache;
	    PyObject *values;
    };
    khpair_t *kh; // key & value pair
    uint8_t  *kind; // kind of keys in dict
} entry_t;

typedef struct {
    PyObject_HEAD
    entry_t entries; 
    ssize_t capacity;
    ssize_t group_capacity;
    ssize_t size;
} QPyDictObject;

#endif // QPy_TYPES_H
