#ifndef QPy_TYPES_H
#define QPy_TYPES_H

#ifndef NO_PYAPI
#ifndef Py_SSIZE_T_CLEAN
#define Py_SSIZE_T_CLEAN
#endif
#include <Python.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define QPy_T_SSIZE Py_T_PYSSIZET

#ifndef UINTPTR_MAX
typedef uintptr_t unsigned long int
#endif

#if NO_PYAPI
typedef PyObject   Type;
typedef Py_ssize_t ssize_t;
typedef Py_hash_t  hash_t;
#else
typedef void *   Type;
typedef long int ssize_t;
typedef unsigned long int hash_t
#endif

typedef struct {
    Type   *key;
    hash_t  hash;
} khpair_t;

typedef struct {
    struct 
    {
	    uint8_t *cache;
	    Type   **values;
    };
    khpair_t *kh; // key & value pair
    uint8_t  *kind; // kind of keys in dict
} entry_t;

typedef struct {
#ifndef NO_PYAPI
    PyObject_HEAD
#endif
    entry_t entries; 
    ssize_t capacity;
    ssize_t group_capacity;
    ssize_t size;
#ifdef NO_PYAPI
    hash_t (hash *)(Type *key);
    int    (cmp  *)(Type *t, Type *u);
#endif
} QPyDictObject;

#endif // QPy_TYPES_H
