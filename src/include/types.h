#ifndef QPy_TYPES_H
#define QPy_TYPES_H

#ifndef NO_PyAPI
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

#ifndef NO_PyAPI
typedef PyObject Type;
typedef Py_ssize_t ssize_t;
typedef Py_hash_t hash_t;
#else
typedef void Type;
//typedef long int ssize_t;
typedef unsigned long int hash_t;
#endif

typedef uint8_t cache_t;

typedef hash_t (* hashfunc_t)(Type *key);
typedef int    (* cmpfunc_t)(Type *t, Type *u);
typedef void   (* clearfunc_t)(Type *key, Type *value, void *arg);

typedef struct
{
    Type  *key;
    hash_t hash;
} khpair_t;

typedef struct
{
    const Type  *key;
    const hash_t hash;
} ckhpair_t;

typedef struct
{
    struct
    {
        cache_t *cache;
        Type   **values;
    };
    khpair_t *kh; // key & value pair
} entry_t;

typedef struct
{
#ifndef NO_PyAPI
    PyObject_HEAD
#endif
    entry_t   entries;

#ifdef NO_PYAPI
    hash_t (* hash )(Type *key);
    void   (* clear)(Type *key, Type *value, void *arg);
#endif
    int    (* cmp  )(Type *t, Type *u);

    size_t  capacity;
    size_t  group_capacity;
    size_t  used_size; // number of put entries
    size_t  max_size;   // maximum size, no resize

    uint8_t flags; // internal use only (key kind, malloc)
} QPyDictObject;

#endif
#endif // QPy_TYPES_H
