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

#if NO_PyAPI
typedef PyObject Type;
typedef Py_ssize_t ssize_t;
typedef Py_hash_t hash_t;
#else
typedef void     Type;
typedef long int ssize_t;
typedef unsigned long int hash_t
#endif

typedef uint8_t cache_t;

typedef hash_t (hashfunc_t  *)(Type *key);
typedef int    (cmpfunc_t   *)(Type *t, Type *u);
typedef void   (clearfunc_t *)(Type *key, Type *value, void *arg);

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
    uint8_t   kind; // kind of keys in dict
} entry_t;

typedef struct
{
#ifndef NO_PyAPI
    PyObject_HEAD
#endif
    entry_t   entries;
#ifdef NO_PYAPI
    hashfunc_t  hash;
    cmpfunc_t   cmp;
    clearfunc_t clear;
#endif
    uintmax_t dict_id;
    ssize_t   capacity;
    ssize_t   group_capacity;
    ssize_t   used_size; // number of put entries
    size_t    max_size;   // maximum size, no resize
    float     lf;
} QPyDictObject;

typedef struct
{
    ssize_t  i;
    ssize_t  size;
    cache_t *group;
    Type **kh;
    Type **val;
} visit_t;

#define y8(y) y, y, y, y, y, y, y, y
static const uint8_t
empty_tag_full_group[NGROUP_MAX] = {
    y8(EMPTY_ENTRY), y8(EMPTY_ENTRY),
    y8(EMPTY_ENTRY), y8(EMPTY_ENTRY)
};
#undef y8
#endif

#endif // QPy_TYPES_H
